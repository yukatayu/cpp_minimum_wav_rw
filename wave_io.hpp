#pragma once
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <optional>

namespace WavFile {

#pragma pack(push, 1)
	struct Riff {
		std::uint32_t str_riff;  // RIFF = 0x46464952
		std::uint32_t size;
		std::uint32_t str_wave;  // WAVE = 0x45564157
		std::uint32_t str_fmt;   // fmt  = 0x20746D66

		std::uint32_t linear;         // linear -> 0x00000010
		std::uint16_t format;         // uncompressed -> 0x0001
		std::uint16_t channel_count;  // 1 or 2 であってほしい
		std::uint32_t sampling_rate;  // 44.1kHz -> 0x0000AC44
		std::uint32_t avg_byte;       // sampling_rate * block_size

		std::uint16_t block_size;   // channel_count * sample_size / 8
		std::uint16_t sample_size;  // 8 or 16 であってほしい
	};

	struct ChunkHead {
		std::uint32_t type;
		std::uint32_t size;
	};
#pragma pack(pop)

	template<class D>
	std::pair<Riff, std::optional<std::vector<D>>> read(std::string file_name) {

		std::optional<std::vector<D>> data;
		std::ifstream input_file(file_name, std::ios::binary);
		std::vector<char> input_buf(std::istreambuf_iterator<char>{ input_file }, {});
	
		// 生ポ取り出し
		char* input_ptr = input_buf.data();
		const char* input_ptr_end = input_buf.data() + input_buf.size();
		assert(sizeof(*input_ptr) == 1);

		// まずヘッダ読み込み
		Riff riff = *reinterpret_cast<Riff*>(input_ptr);

		{  // ひたすら assert
			assert(riff.str_riff == 0x46464952);
			assert(riff.size     == input_buf.size() - 8);
			assert(riff.str_wave == 0x45564157);
			assert(riff.str_fmt  == 0x20746D66);

			assert(riff.linear == 0x00000010);
			assert(riff.format == 0x0001);
			assert(riff.channel_count == 1 || riff.channel_count == 2);  // とりあえず
			assert(riff.sampling_rate == 0x0000AC44);
			assert(riff.avg_byte == riff.sampling_rate * riff.block_size);

			assert(riff.block_size == riff.channel_count * riff.sample_size / 8);
			assert(riff.sample_size == sizeof(D) * 8);

			input_ptr += sizeof(Riff);
		}

		// data のチャンクを探して持ってくる
		while (input_ptr < input_ptr_end) {
			ChunkHead* chunk_head = reinterpret_cast<ChunkHead*>(input_ptr);
			if (chunk_head->type == 0x61746164) {  // data = 0x61746164
				assert(chunk_head->size % sizeof(D) == 0);
				data =
					std::vector<D>(
						reinterpret_cast<const D*>(input_ptr),
						reinterpret_cast<const D*>(input_ptr + chunk_head->size));
			}
			input_ptr += sizeof(ChunkHead);
			input_ptr += chunk_head->size;
		}

		return { riff, data };
	}

	template<class D>  // D は uint8_t か uint16_t
	void write(std::string file_name, unsigned int channel_count, std::vector<D> data) {
		std::vector<char> output_buf;

		// Riffの生成
		Riff riff{};

		riff.str_riff = 0x46464952;
		riff.str_wave = 0x45564157;
		riff.str_fmt  = 0x20746D66;

		riff.linear = 0x00000010;
		riff.format = 0x0001;
		riff.channel_count = channel_count;
		riff.sampling_rate = 0x0000AC44;

		riff.sample_size = sizeof(D) * 8;
		riff.block_size  = riff.channel_count * riff.sample_size / 8;
		riff.avg_byte    = riff.sampling_rate * riff.block_size;

		// バッファに書き込み
		output_buf.resize(sizeof(Riff) + sizeof(ChunkHead) + sizeof(short) * data.size());
		char* output_ptr = output_buf.data();
		char* const output_ptr_end = output_buf.data() + output_buf.size();
	
		//    // riff
		std::memcpy(output_ptr, &riff, sizeof(Riff));
		output_ptr += sizeof(Riff);

		//    // data_head
		ChunkHead data_head{ 0x61746164, static_cast<std::uint32_t>(data.size() * sizeof(short)) };
		std::memcpy(output_ptr, &data_head, sizeof(ChunkHead));
		output_ptr += sizeof(ChunkHead);

		//    // data
		std::memcpy(output_ptr, reinterpret_cast<char*>(data.data()), data.size() * sizeof(short));
		output_ptr += data.size() * sizeof(short);

		assert(output_ptr <= output_ptr_end);

		// サイズだけ最後に書き込み
		reinterpret_cast<Riff*>(output_buf.data())->size = static_cast<std::uint32_t>(output_buf.size() - 8);

		// ファイルに書き込み
		std::ofstream output_file(file_name, std::ios::binary);
		output_file.write(output_buf.data(), output_buf.size());
	}
}
