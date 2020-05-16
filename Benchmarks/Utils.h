#include <SLabCoreLib/SLabTypes.h>
#include <SLabCoreLib/SysSpecifics.h>
#include <SLabCoreLib/Vectors.h>

#include <vector>

size_t MakeSize(size_t count);

unique_aligned_buffer<float> MakeFloats(size_t count);
unique_aligned_buffer<float> MakeFloats(size_t count, float value);
unique_aligned_buffer<ElementIndex> MakeElementIndices(ElementIndex maxElementIndex, size_t count);
unique_aligned_buffer<vec2f> MakeVectors(size_t count);
