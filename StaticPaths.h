#pragma once
#include <vector>

namespace Echo {
	//
	// 

	const std::vector<std::vector<uint16>> STATIC_PATHS = {
		// 格式：{道路ID序列}
		{0},  // Path[0]: Road[0]
		{0, 4},  // Path[1]: Road[0]->Road[4]
		{0, 4, 15},  // Path[2]: Road[0]->Road[4]->Road[15]
		{1},  // Path[3]: Road[1]
		{2},  // Path[4]: Road[2]
		{2, 22},  // Path[5]: Road[2]->Road[22]
		{3},  // Path[6]: Road[3]
		{4},  // Path[7]: Road[4]
		{4, 15},  // Path[8]: Road[4]->Road[15]
		{5},  // Path[9]: Road[5]
		{5, 16},  // Path[10]: Road[5]->Road[16]
		{6},  // Path[11]: Road[6]
		{6, 19},  // Path[12]: Road[6]->Road[19]
		{6, 20},  // Path[13]: Road[6]->Road[20]
		{7},  // Path[14]: Road[7]
		{8},  // Path[15]: Road[8]
		{9},  // Path[16]: Road[9]
		{10},  // Path[17]: Road[10]
		{11},  // Path[18]: Road[11]
		{12},  // Path[19]: Road[12]
		{13},  // Path[20]: Road[13]
		{13, 21},  // Path[21]: Road[13]->Road[21]
		{14},  // Path[22]: Road[14]
		{14, 22},  // Path[23]: Road[14]->Road[22]
		{15},  // Path[24]: Road[15]
		{16},  // Path[25]: Road[16]
		 {17},  // Path[26]: Road[17]
		{18},  // Path[27]: Road[18]
		{19},  // Path[28]: Road[19]
		{20},  // Path[29]: Road[20]
		{21},  // Path[30]: Road[21]
		{22},  // Path[31]: Road[22]

		// 运行后，从日志复制实际生成的路径到这里
		// 然后在 Traffic::generateAllPaths() 中切换到: m_allPaths = STATIC_PATHS;
	};

} // namespace Echo
