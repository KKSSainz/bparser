#include <stdio.h>
#include <chrono>
#include <iostream>
#include "..//third_party/VCL_v2/vectorclass.h"
#include "..//third_party/VCL_v2/instrset_detect.cpp"

template<typename VecType>
void printVector(const VecType & v, const char * prefix)
{
    bool first = true;
    std::cout << prefix << "(";
    for(int i = 0; i < VecType::size(); i++)
    {
        if (first)
        {
            std::cout << v[i];
            first = false;
            continue;
        }

        std::cout << " ; " << v[i];
    }
    std::cout << ")" << std::endl;
}

template <typename VecType>
void eval(VecType &res, VecType a, VecType b)
{
	// as_bool(res) = a < b;
    res = as_double(a < b);
}

template<typename bool_type> struct b_to_d;

template<>
struct b_to_d<Vec2db> {
    typedef Vec2d double_type;
};

template<>
struct b_to_d<Vec4db> {
    typedef Vec4d double_type;
};

template<>
struct b_to_d<Vec8db> {
    typedef Vec8d double_type;
};

template<typename bool_type> union MaskDouble;

template<>
union MaskDouble<Vec2db> {
	Vec2db	mask;
	Vec2d  value;
};

template<>
union MaskDouble<Vec4db> {
	Vec4db	mask;
	Vec4d  value;
};

template<>
union MaskDouble<Vec8db> {
	Vec8db	mask;
	Vec8d  value;
};

template<typename bool_type>
inline typename b_to_d<bool_type>::double_type as_double(bool_type in) {
    MaskDouble<bool_type> x = {in};
    return x.value;
}

template<typename double_type> struct d_to_b;

template<>
struct d_to_b<Vec2d> {
    typedef Vec2db bool_type;
};

template<>
struct d_to_b<Vec4d> {
    typedef Vec4db bool_type;
};

template<>
struct d_to_b<Vec8d> {
    typedef Vec8db bool_type;
};

template<typename double_type> union DoubleMask;

template<>
union DoubleMask<Vec2d> {
	Vec2d  value;
	Vec2db	mask;
};

template<>
union DoubleMask<Vec4d> {
	Vec4d  value;
	Vec4db	mask;
};

template<>
union DoubleMask<Vec8d> {
	Vec8d  value;
	Vec8db	mask;
};


template<typename double_type>
inline typename d_to_b<double_type>::bool_type as_bool(double_type in) {
    DoubleMask<double_type> x = {in};
    return x.mask;
}

// template<typename bool_type>
// typename b_to_d<bool_type>::double_type as_double(bool_type &in) {
//     return *((typename b_to_d<bool_type>::double_type *)(&in));
// }

// template<typename double_type>
// typename d_to_b<double_type>::bool_type as_bool(double_type &in) {
//     return *((typename d_to_b<double_type>::bool_type *)(&in));
// }

//#define MAX_VECTOR_SIZE 256
int main()
{
    Vec2d a(0.0, 5.0);
    Vec2d b(1.0, 4.0);
    Vec2d r;
    Vec2db x;
    
    x = a < b;
    r = as_double(x);

    printVector<Vec2db>(x, "x");
    printVector<Vec2d>(r, "r");

///////////////////////////////////////////////////////////////
    Vec4d aa(0.0, 1.0, 20.0, 30.0);
    Vec4d bb(10.0, 20.0, 3.0, 4.0);
    Vec4d rr;

    Vec4d cc;
    Vec4d &cc_ref = cc;
    cc_ref = aa < bb;

    eval<Vec4d>(rr, aa, bb);

    printVector<Vec4d>(rr, "rr");
    printVector<Vec4db>(as_bool(rr), "rr");
    printVector<Vec4d>(cc_ref, "cc_ref");

    Vec4db xx = aa < bb;
    cc_ref = xx;

    printVector<Vec4d>(cc_ref, "cc_ref");

    cc_ref = as_double(xx);

    printVector<Vec4d>(cc_ref, "cc_ref");

////////////////////////////////////////////////////////////////////////
    Vec8d aaa(0.0, 1.0, 20.0, 30.0, 0.0, 1.0, 20.0, 30.0);
    Vec8d bbb(10.0, 20.0, 3.0, 4.0, 0.0, 1.0, 20.0, 30.0);
    Vec8d rrr;
    Vec8db xxx;

    xxx = aaa < bbb;

    printVector<Vec8db>(xxx, "xxx");

    rrr = as_double(xxx);

    printVector<Vec8d>(rrr, "rrr");

    as_bool(rrr) = xxx;

    printVector<Vec8db>(as_bool(rrr), "rrr");

    Vec8d ccc;
    Vec8d &ccc_ref = ccc;
    ccc_ref = as_double(aaa < bbb);

    printVector<Vec8d>(ccc_ref, "ccc_ref");
    printVector<Vec8db>(as_bool(ccc_ref), "ccc_ref");

    eval<Vec8d>(rrr, aaa, bbb);

    printVector<Vec8d>(rrr, "rrr");

    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < 20000000; i++)
    {
        eval<Vec2d>(r, a, b);
        eval<Vec4d>(rr, aa, bb);
        eval<Vec8d>(rrr, aaa, bbb);   
    }

    auto end_time = std::chrono::high_resolution_clock::now();

	double time  = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
    printf("time elapsed: %f \n", time);

    return 0;

    // typedef std::conditional<INSTRSET >= 7, Vec4d, Vec2d>::type tmp;
    // typedef std::conditional<INSTRSET >= 9, Vec8d, tmp>::type VecType;

    // VecType res;

    // start_time = std::chrono::high_resolution_clock::now();
    // for (int i = 1; i < 1000000; i++)
    // {
    //     VecType test(1.2, 2.4, 3.6, 4.8);
    //     VecType trst(5.6, 4.2, 23.4, 234.6);

    //     res = test + trst;
    // }
    // end_time = std::chrono::high_resolution_clock::now();

	// time  = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - start_time).count();
    // printf("%f \n", time);


    // int is = instrset_detect();
    // int instrs = INSTRSET;
    // printf("%i ", is);
    // printf("%i ", instrs);
    // printf("%i ", INSTRSET);
    // printf("%i ", MAX_VECTOR_SIZE);
    // printf("%li ", sizeof(Vec4d));
    // printf("%li ", sizeof(Vec4db));
    // printf("\nResult res_test = ");
    // for(int i = 0; i < res.size(); i++)
    // {
    //     printf("%f ", res[i]);
    // }

    // return 0;
}


// // Labels are useful for searching through assembly listings.
// test1_start:
//     c = a + b;
// test1_end:
//     printVector(c, "c:" );

// test2_start:
//     d = a + 2 * b;
// test2_end:
//     printVector(d, "d:" );

// test3_start:
//     e = a * b;
// test3_end:
//     printVector(e, "e:" );

