/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 *
 * The layout of a Pack File is as follows:
 *
 * - 8 byte magic header, containing the chars "LiOnHeAd"
 * - A number of blocks, one having the name MESHES, INFO,
 *   LHAudioBankSampleTable and the rest containing textures. The blocks are
 *   concatenated one after the other. (see below)
 *
 * ------------------------- start of block ------------------------------------
 *
 * - 36 byte header containing:
 *         32 char name
 *         4 byte size of the block
 * - arbitrary size body based on size in head
 *
 * ------------------------- end of file ---------------------------------------
 *
 * The layout of a MESHES block is as follows:
 *
 * - 4 byte magic header, containing the chars "MKJC"
 * - 4 byte int, containing the number of L3D meshes contained. The meshes are
 *   concatenated one after the other within the block (see below).
 *
 * -----------------------------------------------------------------------------
 *
 * The layout of a Body block is as follows:
 *
 * - 4 byte magic header, containing the chars "MKJC"
 * - 4 byte int, containing the number of ANM animations contained. The
 *   animation matadata are concatenated one after the other within the block
 *   (see below).
 *
 * ------------------------- start of animation metadata -----------------------
 *
 * - 4 bytes offset into the block
 * - 4 bytes of unknown data
 *
 * ------------------------- end of Body block ---------------------------------
 *
 * - 4 bytes offset into the block
 * - arbitrary size of body based on the size of an L3DMesh
 *
 * ------------------------- end of MESHES block -------------------------------
 *
 * The layout of a INFO block is as follows:
 *
 * - 4 byte int, containing the number of textures in the block.
 * - 8 byte look-up table * number of textures, containing
 *         block id - integer whose hexadecimal string corresponds of a block in
 *                    the file.
 *         unknown - TODO: Maybe type? Maybe layers?
 *
 * ------------------------- end of INFO block ---------------------------------
 *
 * The layout of a LHAudioBankSampleTable block is as follows:
 *
 * - 2 byte int, containing the number of sound samples in the block.
 * - 2 byte int, unknown
 * - 640 byte audio metadata * number of sound samples, containing
 *         name - 256 characters
 *         unknown - TODO: 4 bytes
 *         id - 4 bytes
 *         if the sample is a bank - 4 bytes
 *         the size of the sample - 4 bytes
 *         the offset of the sample - 4 bytes
 *         if the sample is a clone - 4 bytes
 *         group - 2 bytes
 *         atmospheric group - 2 bytes
 *         4 unknowns - TODO: 4 bytes, 4 bytes, 2 bytes, 2 bytes
 *         sample rate - 4 bytes
 *         5 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes, 2 bytes, 4 bytes
 *         start - 4 bytes
 *         end - 4 bytes
 *         description - 256 characters
 *         priority - 2 bytes
 *         3 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes
 *         loop - 2 bytes
 *         start - 2 bytes
 *         pan - 1 byte
 *         unknowns - TODO: 2 bytes
 *         position - 3 * 32 bit floats
 *         volume - 1 byte
 *         user parameters - 2 bytes
 *         pitch - 2 bytes
 *         unknown - 2 bytes
 *         pitchDeviation - 2 bytes
 *         unknown - 2 bytes
 *         minimum distance - 32 bit float
 *         maximum distance - 32 bit float
 *         scale - 32 bit float
 *         loop type - 16 bit enumeration where: 0 is None, 1 is Restart, 2 is Once and 3 is Overlap
 *         3 unknowns - TODO: 2 bytes, 2 bytes, 2 bytes
 *         atmosphere - 2 bytes
 *
 * ------------------------- end of LHAudioBankSampleTable block ---------------
 *
 * The layout of a texture block is as follows:
 *
 * - 16 byte header containing 4 ints
 *         size - size of the block
 *         block id - integer whose hexadecimal string corresponds of a block in
 *                    the file.
 *         type - TODO: unknown, maybe it corresponds to the unknown in lookup
 *         dds file size - size of the dds file minus magic number
 * - variable size dds file without the first 4 byte magic number
 *
 * The base game uses DXT1 and DXT3 textures
 * The Creature Isle also uses DXT5 textures
 *
 * ------------------------- end of texture block ------------------------------
 *
 */

#include "PackFile.h"

#include <cassert>
#include <cstring>

#include <fstream>

using namespace openblack::pack;

namespace
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - pos_type(static_cast<off_type>(0)), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, public std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};

struct PackBlockHeader
{
	static constexpr uint32_t k_BlockNameSize = 0x20;
	std::array<char, k_BlockNameSize> blockName;
	uint32_t blockSize;
};

/// Magic Key Jean-Claude Cottier
constexpr const std::array<char, 4> k_BlockMagic = {'M', 'K', 'J', 'C'};
} // namespace

/// Error handling
void PackFile::Fail(const std::string& msg)
{
	throw std::runtime_error("Pack Error: " + msg + "\nFilename: " + _filename.string());
}

void PackFile::ReadBlocks(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	std::array<char, k_Magic.size()> magic;
	if (fsize < magic.size() + sizeof(PackBlockHeader))
	{
		Fail("File too small to be a valid Pack file.");
	}

	// First 8 bytes
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_Magic.data(), magic.size()) != 0)
	{
		Fail("Unrecognized Pack header");
	}

	if (fsize < static_cast<std::size_t>(stream.tellg()) + sizeof(PackBlockHeader))
	{
		Fail("File too small to contain any blocks.");
	}

	PackBlockHeader header;
	while (fsize - sizeof(PackBlockHeader) > static_cast<std::size_t>(stream.tellg()))
	{
		stream.read(reinterpret_cast<char*>(&header), sizeof(PackBlockHeader));

		if (_blocks.contains(header.blockName.data()))
		{
			Fail(std::string("Duplicate block name: ") + header.blockName.data());
		}

		_blocks[std::string(header.blockName.data())] = std::vector<uint8_t>(header.blockSize);
		stream.read(reinterpret_cast<char*>(_blocks[header.blockName.data()].data()), header.blockSize);
	}

	if (fsize < static_cast<std::size_t>(stream.tellg()))
	{
		Fail("File not evenly split into whole blocks.");
	}
}

void PackFile::ResolveInfoBlock()
{
	if (!HasBlock("INFO"))
	{
		Fail("no INFO block in mesh pack");
	}

	auto data = GetBlock("INFO");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	uint32_t totalTextures;
	stream.read(reinterpret_cast<char*>(&totalTextures), sizeof(uint32_t));

	// Read lookup
	_infoBlockLookup.resize(totalTextures);
	stream.read(reinterpret_cast<char*>(_infoBlockLookup.data()), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));
}

void PackFile::ResolveBodyBlock()
{
	if (!HasBlock("Body"))
	{
		Fail("no Body block in anim pack");
	}

	auto data = GetBlock("Body");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	// Greetings Jean-Claude Cottier
	std::array<char, k_BlockMagic.size()> magic;
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_BlockMagic.data(), magic.size()) != 0)
	{
		Fail("Unrecognized Body Block header");
	}

	uint32_t totalAnimations;
	stream.read(reinterpret_cast<char*>(&totalAnimations), sizeof(uint32_t));

	// Read lookup offsets
	_bodyBlockLookup.resize(totalAnimations);
	stream.read(reinterpret_cast<char*>(_bodyBlockLookup.data()), _bodyBlockLookup.size() * sizeof(_bodyBlockLookup[0]));
}

void PackFile::ResolveAudioBankSampleTableBlock()
{
	if (!HasBlock("LHAudioBankSampleTable"))
	{
		Fail("no LHAudioBankSampleTable block in sad pack");
	}

	auto data = GetBlock("LHAudioBankSampleTable");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(uint32_t))
	{
		Fail("Audio bank block cannot fit sample count: " + std::to_string(fsize) + " < " + std::to_string(sizeof(uint8_t)));
	}

	uint16_t sampleCount;
	stream.read(reinterpret_cast<char*>(&sampleCount), sizeof(sampleCount));

	uint16_t unknown;
	stream.read(reinterpret_cast<char*>(&unknown), sizeof(unknown));

	if (sampleCount == 0)
	{
		Fail("There are no sound entries");
	}

	_audioSampleHeaders.resize(sampleCount);

	if (fsize != sizeof(uint32_t) + _audioSampleHeaders.size() * sizeof(_audioSampleHeaders[0]))
	{
		Fail("Cannot fit all " + std::to_string(sampleCount) + " sample headers");
	}

	stream.read(reinterpret_cast<char*>(_audioSampleHeaders.data()),
	            _audioSampleHeaders.size() * sizeof(_audioSampleHeaders[0]));
}

void PackFile::ExtractTexturesFromBlock()
{
	G3DTextureHeader header;
	constexpr uint32_t blockNameSize = 0x20;
	std::array<char, blockNameSize> blockName;
	for (const auto& item : _infoBlockLookup)
	{
		// Convert int id to string representation as hexadecimal key
		std::snprintf(blockName.data(), blockName.size(), "%x", item.blockId);

		if (!HasBlock(blockName.data()))
		{
			Fail(std::string("Required texture block \"") + blockName.data() + "\" missing.");
		}

		auto stream = GetBlockAsStream(blockName.data());

		stream->read(reinterpret_cast<char*>(&header), sizeof(header));
		std::vector<uint8_t> dds(header.size);
		stream->read(reinterpret_cast<char*>(dds.data()), dds.size() * sizeof(dds[0]));

		if (header.id != item.blockId)
		{
			Fail("Texture block id is not the same as block id");
		}

		if (_textures.contains(blockName.data()))
		{
			Fail("Duplicate texture extracted");
		}

		imemstream ddsStream(reinterpret_cast<const char*>(dds.data()), dds.size());

		DdsHeader ddsHeader;
		ddsStream.read(reinterpret_cast<char*>(&ddsHeader), sizeof(DdsHeader));

		// Verify the header to validate the DDS file
		if (ddsHeader.size != sizeof(DdsHeader) || ddsHeader.format.size != sizeof(DdsPixelFormat))
		{
			Fail("Invalid DDS header sizes");
		}

		// Handle cases where this field is not provided
		// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
		// Some Creature Isle DXT5 textures lack this field
		if (ddsHeader.pitchOrLinearSize == 0)
		{
			// The block-size is 8 bytes for DXT1, BC1, and BC4 formats, and 16 bytes for other block-compressed formats
			int blockSize;
			auto format = std::string(ddsHeader.format.fourCC.data(), ddsHeader.format.fourCC.size());
			if (format == "DXT1" || format == "BC1" || format == "BC4")
			{
				blockSize = 8;
			}
			else
			{
				blockSize = 16;
			}

			ddsHeader.pitchOrLinearSize = ((ddsHeader.width + 3) / 4) * ((ddsHeader.height + 3) / 4) * blockSize;
		}

		std::vector<uint8_t> dssTexels(ddsHeader.pitchOrLinearSize);
		ddsStream.read(reinterpret_cast<char*>(dssTexels.data()), dssTexels.size());

		_textures[blockName.data()] = {header, ddsHeader, dssTexels};
	}
}

void PackFile::ExtractAnimationsFromBlock()
{
	auto data = GetBlock("Body");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());

	// Read lookup
	constexpr uint32_t blockNameSize = 0x20;
	constexpr uint32_t animationHeaderSize = 0x54;

	std::array<char, blockNameSize> blockName;
	_animations.resize(_bodyBlockLookup.size());
	for (uint32_t i = 0; i < _bodyBlockLookup.size(); ++i)
	{
		snprintf(blockName.data(), blockNameSize, "Julien%d", i);
		if (!HasBlock(blockName.data()))
		{
			Fail(std::string("Required texture block \"") + blockName.data() + "\" missing.");
		}

		auto animationData = GetBlock(blockName.data());
		_animations[i].resize(animationHeaderSize + animationData.size());

		stream.seekg(_bodyBlockLookup[i].offset);
		stream.read(reinterpret_cast<char*>(_animations[i].data()), animationHeaderSize);
		memcpy(_animations[i].data() + animationHeaderSize, animationData.data(), animationData.size());
	}
}

void PackFile::ExtractSoundsFromBlock()
{
	if (!HasBlock("LHAudioWaveData"))
	{
		Fail("No LHAudioWaveData block in sad pack");
	}

	auto data = GetBlock("LHAudioWaveData");
	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());
	//	auto isSector = false;
	//	auto isPrevSector = false;

	_audioSampleData.resize(_audioSampleHeaders.size());
	for (int i = 0; const auto& sample : _audioSampleHeaders)
	{
		if (sample.offset > data.size())
		{
			Fail("Sound sample offset points to beyond file");
		}
		if (sample.offset + sample.size > data.size())
		{
			Fail("Sound sample size exceeds LHAudioWaveData size");
		}

		_audioSampleData[i].resize(sample.size);

		stream.seekg(sample.offset);
		stream.read(reinterpret_cast<char*>(_audioSampleData[i].data()), sample.size);

		++i;
	}
}

void PackFile::ResolveMeshBlock()
{
	if (!HasBlock("MESHES"))
	{
		Fail("no MESHES block in mesh pack");
	}
	auto data = GetBlock("MESHES");

	imemstream stream(reinterpret_cast<const char*>(data.data()), data.size());
	// Greetings Jean-Claude Cottier
	std::array<char, k_BlockMagic.size()> magic;
	stream.read(magic.data(), magic.size());
	if (std::memcmp(magic.data(), k_BlockMagic.data(), magic.size()) != 0)
	{
		Fail("Unrecognized Mesh Block header");
	}

	uint32_t meshCount;
	stream.read(reinterpret_cast<char*>(&meshCount), sizeof(meshCount));
	std::vector<uint32_t> meshOffsets(meshCount);
	stream.read(reinterpret_cast<char*>(meshOffsets.data()), meshOffsets.size() * sizeof(meshOffsets[0]));

	_meshes.resize(meshOffsets.size());
	for (std::size_t i = 0; i < _meshes.size(); i++)
	{
		auto size = (i == _meshes.size() - 1 ? data.size() : meshOffsets[i + 1]) - meshOffsets[i];
		_meshes[i].resize(size);
		stream.read(reinterpret_cast<char*>(_meshes[i].data()), _meshes[i].size() * sizeof(_meshes[i][0]));
	}
}

void PackFile::WriteBlocks(std::ostream& stream) const
{
	assert(!_isLoaded);

	// Magic number
	stream.write(k_Magic.data(), k_Magic.size());

	PackBlockHeader header;

	for (const auto& [name, contents] : _blocks)
	{
		std::snprintf(header.blockName.data(), header.blockName.size(), "%s", name.c_str());
		header.blockSize = static_cast<uint32_t>(contents.size() * sizeof(contents[0]));

		stream.write(reinterpret_cast<const char*>(&header), sizeof(header));
		stream.write(reinterpret_cast<const char*>(contents.data()), header.blockSize);
	}
}

void PackFile::CreateTextureBlocks()
{
	// TODO(bwrsandman): Loop through every texture and create a block with
	//                   their id. Then fill in the look-up table.
	assert(_isLoaded);
	assert(false);
}

void PackFile::CreateRawBlock(const std::string& name, std::vector<uint8_t>&& data)
{
	if (HasBlock(name))
	{
		Fail("Pack file already has a " + name + " block");
	}

	_blocks[name] = std::move(data);
}

void PackFile::CreateMeshBlock()
{
	if (HasBlock("MESHES"))
	{
		Fail("Mesh pack already has a MESHES block");
	}

	size_t offset = 0;
	std::vector<uint8_t> contents;

	auto meshCount = static_cast<uint32_t>(_meshes.size());
	contents.resize(k_BlockMagic.size() + sizeof(meshCount));

	std::memcpy(contents.data() + offset, k_BlockMagic.data(), k_BlockMagic.size());
	offset += k_BlockMagic.size();
	std::memcpy(contents.data() + offset, &meshCount, sizeof(meshCount));
	offset += sizeof(meshCount);

	contents.resize(offset + sizeof(uint32_t) * meshCount);
	if (meshCount > 0)
	{
		const auto previousOffset = offset;
		offset += sizeof(uint32_t) * meshCount;
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			auto* meshOffsets = reinterpret_cast<uint32_t*>(&contents[previousOffset]);
			meshOffsets[i] = static_cast<uint32_t>(offset);
			offset += _meshes[i].size();
		}
		contents.resize(offset);
		for (size_t i = 0; i < _meshes.size(); ++i)
		{
			if (!_meshes[i].empty())
			{
				auto meshOffset = reinterpret_cast<uint32_t*>(&contents[previousOffset])[i];
				std::memcpy(&contents[meshOffset], _meshes[i].data(), _meshes[i].size() * sizeof(_meshes[i][0]));
			}
		}
	}

	_blocks["MESHES"] = std::move(contents);
}

void PackFile::InsertMesh(std::vector<uint8_t> data)
{
	_meshes.emplace_back(std::move(data));
}

void PackFile::CreateInfoBlock()
{
	if (HasBlock("INFO"))
	{
		Fail("Mesh pack already has an INFO block");
	}

	uint32_t offset = 0;
	std::vector<uint8_t> contents;

	auto totalTextures = static_cast<uint32_t>(_infoBlockLookup.size());
	contents.resize(sizeof(totalTextures) + _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	std::memcpy(contents.data() + offset, &totalTextures, sizeof(totalTextures));
	offset += static_cast<uint32_t>(sizeof(totalTextures));

	std::memcpy(contents.data() + offset, _infoBlockLookup.data(), _infoBlockLookup.size() * sizeof(_infoBlockLookup[0]));

	_blocks["INFO"] = std::move(contents);
}

void PackFile::CreateBodyBlock()
{
	if (HasBlock("Body"))
	{
		Fail("Pack already has an Body block");
	}

	std::vector<uint8_t> contents;

	_blocks["Body"] = std::move(contents);
}

PackFile::PackFile() = default;
PackFile::~PackFile() = default;

void PackFile::ReadFile(std::istream& stream)
{
	ReadBlocks(stream);
	// Mesh pack
	if (HasBlock("INFO"))
	{
		ResolveInfoBlock();
		ExtractTexturesFromBlock();
		ResolveMeshBlock();
	}
	// Anim pack
	if (HasBlock("Body"))
	{
		ResolveBodyBlock();
		ExtractAnimationsFromBlock();
	}
	// Sound pack
	if (HasBlock("LHAudioBankSampleTable"))
	{
		ResolveAudioBankSampleTableBlock();
		// ResolveFileSegmentBankBlock();
		ExtractSoundsFromBlock();
	}

	_isLoaded = true;
}

void PackFile::Open(const std::filesystem::path& file)
{
	_filename = file;

	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	ReadFile(stream);
}

void PackFile::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using L3DFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void PackFile::Write(const std::filesystem::path& file)
{
	assert(!_isLoaded);

	_filename = file;

	std::ofstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	WriteBlocks(stream);
}

std::unique_ptr<std::istream> PackFile::GetBlockAsStream(const std::string& name) const
{
	const auto& data = GetBlock(name);
	return std::make_unique<imemstream>(reinterpret_cast<const char*>(data.data()), data.size());
}
