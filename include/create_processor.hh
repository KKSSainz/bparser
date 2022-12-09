#include "processor.hh"
#include <stdint.h>
#include "expression_dag.hh"

namespace bparser{

    ProcessorBase * ProcessorBase::create_processor(ExpressionDAG &se, uint vector_size,  uint simd_size, ArenaAllocPtr arena) {
        if (simd_size == 0) {
            simd_size = get_simd_size();
        }

        switch (simd_size) {
            case 2:
            {
                return create_processor_<Vec2d>(se, vector_size, simd_size, arena);
            } break;
            case 4:
            { 
                return create_processor_<Vec4d>(se, vector_size, simd_size, arena);
            } break;
            case 8:
            {
                return create_processor_<Vec8d>(se, vector_size, simd_size, arena);
            } break;
            default:
            {
                return create_processor_<double>(se, vector_size, 1, arena);
            } break;
        }
    }
}