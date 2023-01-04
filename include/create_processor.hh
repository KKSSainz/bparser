#include "processor.hh"
#include "expression_dag.hh"
#include "instrset_detect.cpp"

namespace bparser{

    uint get_simd_size() {
        int i_set = instrset_detect();

        if (i_set >= 9)      // min AVX512F
        {
            return 8;
        }
        else if (i_set >= 8) // min AVX2
        {
            return 4;
        }
        else if (i_set >= 2) // min SSE4.1
        {
            return 2;
        }
        else                // no vectorization
        {
            return 1;
        }
    }

    ProcessorBase * ProcessorBase::create_processor(ExpressionDAG &se, uint vector_size, uint simd_size, ArenaAllocPtr arena) {
        if (simd_size == 0) {
            simd_size = get_simd_size();
        }

        switch (simd_size) {
            case 2:
            {
                // std::cout << "** create processor with SSE4 --" << std::endl;
                return create_processor_<Vec2d>(se, vector_size, simd_size, arena);
            } break;
            case 4:
            { 
                // std::cout << "** create processor with AVX2 --" << std::endl;
                return create_processor_<Vec4d>(se, vector_size, simd_size, arena);
            } break;
            case 8:
            {
                // std::cout << "** create processor with AVX512 --" << std::endl;
                return create_processor_<Vec8d>(se, vector_size, simd_size, arena);
            } break;
            default:
            {
                // std::cout << "** create processor w/o vectorization --" << std::endl;
                return create_processor_<double>(se, vector_size, 1, arena);
            } break;
        }
    }

} // bparser namespace
