/*
 * test_speed_cpp.cc
 *
 *  Created on: Dec 28, 2022
 *      Author: KKSSainz
 */

#include <string>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>

#include "assert.hh"
#include "parser.hh"
#include "test_tools.hh"

#include "arena_alloc.hh"

// Optimized structure, holds data in common arena
struct ExprData {
	ExprData(uint vec_size, uint simd_size)
	: vec_size(vec_size), simd_size(simd_size)
	{
		uint simd_bytes = sizeof(double) * simd_size;

		arena = std::make_shared<bparser::ArenaAlloc>(simd_bytes, 512 * 1012);
		v1 = arena->create_array<double>(vec_size * 3);
		fill_seq(v1, 100, 100 + 3 * vec_size);
		v2 = arena->create_array<double>(vec_size * 3);
		fill_seq(v2, 200, 200 + 3 * vec_size);
		v3 = arena->create_array<double>(vec_size * 3);
		fill_seq(v3, 300, 300 + 3 * vec_size);
		v4 = arena->create_array<double>(vec_size * 3);
		fill_seq(v4, 400, 400 + 3 * vec_size);
		vres = arena->create_array<double>(vec_size * 3);
		fill_const(vres, 3 * vec_size, -100);
		subset = arena->create_array<uint>(vec_size);
		for(uint i=0; i<vec_size/simd_size; i++) subset[i] = i;
		cs1 = 4;
		for (uint i = 0; i < 3; i++)
		{
			cv1[i] = (i+1)*3;
		}
	}	

	~ExprData()
	{}

	std::shared_ptr<bparser::ArenaAlloc> arena;
	uint vec_size;
	uint simd_size;
	double *v1, *v2, *v3, *v4, *vres;
	double cs1;
	double cv1[3];
	uint *subset;

};


// // C++ evaluation of expression "template"
// void expr(ExprData &data) {
// 	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
// 		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
// 			uint j = i_comp + data.simd_size*data.subset[i];
// 			for(uint k = 0; k<data.simd_size; k++) {
// 				double v1 = data.v1[j+k];
// 				double v2 = data.v2[j+k];
// 				double v3 = data.v3[j+k];
// 				double v4 = data.v4[j+k];
// 				double cv1 = data.cv1[i_comp/data.vec_size];
// 				data.vres[j+k] = 
// 			}
// 		}
// 	}
// }


// C++ evaluation of expression "-v1"
void expr0(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = -v1;
			}
		}
	}
}


// C++ evaluation of expression "v1 + v2"
void expr1(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 + v2 ;
			}
		}
	}
}


// C++ evaluation of expression "v1 - v2"
void expr2(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 - v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 * v2"
void expr3(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 * v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 / v2"
void expr4(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 / v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 % v2"
void expr5(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = fmod(v1, v2);
			}
		}
	}
}


// C++ evaluation of expression "v1 == v2"
void expr6(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 == v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 != v2"
void expr7(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 != v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 < v2"
void expr8(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 < v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 <= v2"
void expr9(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 <= v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 > v2"
void expr10(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 > v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 >= v2"
void expr11(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 >= v2;
			}
		}
	}
}


// C++ evaluation of expression "not (v1 == v2)"
void expr12(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = !(v1 == v2);
			}
		}
	}
}


// C++ evaluation of expression "v1 or v2"
void expr13(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 || v2;
			}
		}
	}
}


// C++ evaluation of expression "v1 and v2"
void expr14(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = v1 && v2;
			}
		}
	}
}


// C++ evaluation of expression "abs(v1)"
void expr15(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = abs(v1);
			}
		}
	}
}


// C++ evaluation of expression "sqrt(v1)"
void expr16(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = sqrt(v1);
			}
		}
	}
}


// C++ evaluation of expression "exp(v1)"
void expr17(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = exp(v1);
			}
		}
	}
}


// C++ evaluation of expression "log(v1)"
void expr18(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = log(v1);
			}
		}
	}
}


// C++ evaluation of expression "log10(v1)"
void expr19(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = log10(v1);
			}
		}
	}
}


// C++ evaluation of expression "sin(v1)"
void expr20(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = sin(v1);
			}
		}
	}
}


// C++ evaluation of expression "sinh(v1)"
void expr21(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = sinh(v1);
			}
		}
	}
}


// C++ evaluation of expression "asin(v1)"
void expr22(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = asin(v1);
			}
		}
	}
}


// C++ evaluation of expression "cos(v1)"
void expr23(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = cos(v1);
			}
		}
	}
}


// C++ evaluation of expression "cosh(v1)"
void expr24(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = cosh(v1);
			}
		}
	}
}


// C++ evaluation of expression "acos(v1)"
void expr25(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = acos(v1);
			}
		}
	}
}


// C++ evaluation of expression "tan(v1)"
void expr26(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = tan(v1);
			}
		}
	}
}


// C++ evaluation of expression "tanh(v1)"
void expr27(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = tanh(v1);
			}
		}
	}
}


// C++ evaluation of expression "atan(v1)"
void expr28(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = atan(v1);
			}
		}
	}
}


// C++ evaluation of expression "ceil(v1)"
void expr29(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = ceil(v1);
			}
		}
	}
}


// C++ evaluation of expression "floor(v1)"
void expr30(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = floor(v1);
			}
		}
	}
}


// C++ evaluation of expression "isnan(v1)"
void expr31(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = isnan(v1);
			}
		}
	}
}


// C++ evaluation of expression "isinf(v1)"
void expr32(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = isinf(v1);
			}
		}
	}
}


// C++ evaluation of expression "sgn(v1)"
void expr33(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				data.vres[j+k] = v1 > 0 ? 1.0 : (v1 < 0 ? -1.0 : 0.0);
			}
		}
	}
}


// C++ evaluation of expression "atan2(v1, v2)"
void expr34(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = atan2(v1, v2);
			}
		}
	}
}


// C++ evaluation of expression "v1 ** v2"
void expr35(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = pow(v1, v2);
			}
		}
	}
}


// C++ evaluation of expression "maximum(v1, v2)"
void expr36(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = std::max(v1, v2);
			}
		}
	}
}


// C++ evaluation of expression "minimum(v1, v2)"
void expr37(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = std::min(v1, v2);
			}
		}
	}
}


// C++ evaluation of expression "v3 if v1 == v2 else v4"
void expr38(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double v4 = data.v4[j+k];
				data.vres[j+k] = (v1 == v2) ? v3 : v4;
			}
		}
	}
}


// C++ evaluation of expression "cv1 + v1 + v2 - v3"
void expr39(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double cv1 = data.cv1[i_comp/data.vec_size];
				data.vres[j+k] = cv1 + v1 + v2 - v3;
			}
		}
	}
}


// C++ evaluation of expression "cs1 - v1 + v2 * v3"
void expr40(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				data.vres[j+k] = data.cs1 - v1 + v2 * v3;
			}
		}
	}
}


// C++ evaluation of expression "cs1 * v1 / v2"
void expr41(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				data.vres[j+k] = data.cs1 * v1 / v2;
			}
		}
	}
}


// C++ evaluation of expression "cv1 / v1 * v2 / v3"
void expr42(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double cv1 = data.cv1[i_comp/data.vec_size];
				data.vres[j+k] = cv1 / v1 * v2 / v3;
			}
		}
	}
}


// C++ evaluation of expression "v1 + v2 + v3 + v4"
void expr43(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double v4 = data.v4[j+k];
				data.vres[j+k] = v1 + v2 + v3 + v4;
			}
		}
	}
}


// C++ evaluation of expression "3 * v1 + cs1 * v2 + v3 + 2.5 * v4"
void expr44(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double v4 = data.v4[j+k];
				data.vres[j+k] = 3 * v1 + data.cs1 * v2 + v3 + 2.5 * v4;
			}
		}
	}
}


// C++ evaluation of expression "[v2, v2, v2] @ v1 + v3"
void expr45(ExprData &data) {
	for(uint i_comp=0; i_comp < 3*data.vec_size; i_comp += data.vec_size) {
		for(uint i=0; i<data.vec_size/data.simd_size; ++i) {
			uint j = i_comp + data.simd_size*data.subset[i];
			for(uint k = 0; k<data.simd_size; k++) {
				double v1 = data.v1[j+k];
				double v2 = data.v2[j+k];
				double v3 = data.v3[j+k];
				double v4 = data.v4[j+k];
				data.vres[j+k] = std::min(v1, v2) + std::max(v3, v4);
			}
		}
	}
}




void test_expr(std::string expr, uint block_size, std::string expr_id, std::ofstream& file, void (*f)(ExprData&)) {
	using namespace bparser;
	uint vec_size = 1*block_size;
	uint simd_size = 8; // TODO 

	uint n_repeats = (1024 / block_size) * 10000;

	// TODO: allow changing variable pointers, between evaluations
	// e.g. p.set_variable could return pointer to that pointer
	// not so easy for vector and tensor variables, there are many pointers to set
	// Rather modify the test to fill the
	//uint n_repeats = 1000;

	ExprData data(vec_size, simd_size);

	// C++ evaluation
	auto start_time = std::chrono::high_resolution_clock::now();
	for(uint i_rep=0; i_rep < n_repeats; i_rep++) {
		f(data);
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	double cpp_time  =
			std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();


	// check
	double c_sum = 0;
	for(uint dim=0; dim < 3; dim++) {
		for(uint i=0; i<data.vec_size; i++) {
			double v = data.vres[dim*data.vec_size + i];
			c_sum += v;
		}
	}
	double n_flop = n_repeats * vec_size * 9;


	// std::cout << "=== Parsing of expression: '" << expr << "' ===\n";
	// std::cout << "Repeats    	: " << n_repeats << "\n";
	// std::cout << "Result	  		: " << c_sum << "\n";
	// std::cout << "Block size		: " << block_size << "\n";
	// std::cout << "Experession id	: " << expr_id << "\n";
	// std::cout << "C++ time    	: " << cpp_time << " s \n";
	// std::cout << "C++ avg. time per single execution: " << cpp_time/n_repeats/vec_size*1e9 << " ns \n";
	// std::cout << "c++ FLOPS   	: " << n_flop / cpp_time << "\n";
	// std::cout << "\n";
	// std::cout << "======================================================\n\n";

	file << "C++;"<< expr_id << ";"<< expr << ";"<< c_sum << ";"<< n_repeats << ";"<< block_size << ";" << cpp_time << ";" << cpp_time/n_repeats/vec_size*1e9 << ";" << n_flop/cpp_time << "\n";
 
	// std::cout << "potrebujeme pocet opakovani: " << 10/(cpp_time/n_repeats/vec_size*1e9)*1000 << "\n\n";
}




void test_expression(std::string filename)  {
	std::vector<uint> block_sizes = {64, 256, 1024};

	std::ofstream file;
	if(!filename.empty())
	{
		file.open(filename);
		std::cout << "Outputing to " << filename << "\n";
	}
	else
	{
		file.open("cpp_vystup.csv");
	}

	//header
	file << "Executor;ID;Expression;Result;Repeats;BlockSize;Time;Avg. time per single execution;FLOPS\n";

	for (uint i=0; i<block_sizes.size(); ++i) {
		uint id_counter = 0;
		test_expr("-v1", block_sizes[i], std::to_string(id_counter++), file, &expr0);
		test_expr("v1 + v2", block_sizes[i], std::to_string(id_counter++), file, &expr1);
		test_expr("v1 - v2", block_sizes[i], std::to_string(id_counter++), file, &expr2);
		test_expr("v1 * v2", block_sizes[i], std::to_string(id_counter++), file, &expr3);
		test_expr("v1 / v2", block_sizes[i], std::to_string(id_counter++), file, &expr4);
		test_expr("v1 % v2", block_sizes[i], std::to_string(id_counter++), file, &expr5);

		test_expr("(v1 == v2)", block_sizes[i], std::to_string(id_counter++), file, &expr6);
		test_expr("v1 != v2", block_sizes[i], std::to_string(id_counter++), file, &expr7);
		test_expr("v1 < v2", block_sizes[i], std::to_string(id_counter++), file, &expr8);
		test_expr("v1 <= v2", block_sizes[i], std::to_string(id_counter++), file, &expr9);
		test_expr("v1 > v2", block_sizes[i], std::to_string(id_counter++), file, &expr10);
		test_expr("v1 >= v2", block_sizes[i], std::to_string(id_counter++), file, &expr11);
		test_expr("not (v1 == v2)", block_sizes[i], std::to_string(id_counter++), file, &expr12);
		test_expr("v1 or v2", block_sizes[i], std::to_string(id_counter++), file, &expr13);
		test_expr("v1 and v2", block_sizes[i], std::to_string(id_counter++), file, &expr14);

		test_expr("abs(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr15);
		test_expr("sqrt(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr16);
		test_expr("exp(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr17);
		test_expr("log(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr18);
		test_expr("log10(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr19);
		test_expr("sin(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr20);
		test_expr("sinh(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr21);
		test_expr("asin(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr22);
		test_expr("cos(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr23);
		test_expr("cosh(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr24);
		test_expr("acos(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr25);
		test_expr("tan(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr26);
		test_expr("tanh(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr27);
		test_expr("atan(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr28);
		test_expr("ceil(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr29);
		test_expr("floor(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr30);
		test_expr("isnan(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr31);
		test_expr("isinf(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr32);
		test_expr("sgn(v1)", block_sizes[i], std::to_string(id_counter++), file, &expr33);
		test_expr("atan2(v1, v2)", block_sizes[i], std::to_string(id_counter++), file, &expr34);
		test_expr("v1 ** v2", block_sizes[i], std::to_string(id_counter++), file, &expr35);
		test_expr("maximum(v1, v2)", block_sizes[i], std::to_string(id_counter++), file, &expr36);
		test_expr("minimum(v1, v2)", block_sizes[i], std::to_string(id_counter++), file, &expr37);

		test_expr("v3 if v1 == v2 else v4", block_sizes[i], std::to_string(id_counter++), file, &expr38);
		test_expr("cv1 + v1 + v2 - v3", block_sizes[i], std::to_string(id_counter++), file, &expr39);
		test_expr("cs1 - v1 + v2 * v3", block_sizes[i], std::to_string(id_counter++), file, &expr40);
		test_expr("cs1 * v1 / v2", block_sizes[i], std::to_string(id_counter++), file, &expr41);
		test_expr("cv1 / v1 * v2 / v3", block_sizes[i], std::to_string(id_counter++), file, &expr42);
		test_expr("v1 + v2 + v3 + v4", block_sizes[i], std::to_string(id_counter++), file, &expr43);
		test_expr("3 * v1 + cs1 * v2 + v3 + 2.5 * v4", block_sizes[i], std::to_string(id_counter++), file, &expr44);
		// test_expr("[v2, v2, v2] @ v1 + v3", block_sizes[i], std::to_string(id_counter++), file, &expr45);
	}
}



int main(int argc, char * argv[])
{
	std::string file = "";
	if(argc > 1)
	{
		file = argv[1]; 
	}
	test_expression(file);
}
