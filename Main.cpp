
// g++ -std=c++17 Main.cpp && ./a.out && echo $?

#include "wave_io.hpp"
#include <iostream>
#include <cstdint>

int main() {
	// 読み込み
	auto [riff, maybe_data] = WavFile::read<std::uint16_t>("sample_input.wav");
	auto&& data = std::move(*maybe_data);
	
	// 加工 (前半をカット)
	for (int i = 0; i < data.size() / 2; ++i) {
		const int diff = data.size() / 2;
        data.at(i) = data.at(i + diff);
	}
    data.resize(data.size() / 2);

	// 書き込み
	WavFile::write<std::uint16_t>("sample_output.wav", riff.channel_count, data);
}
