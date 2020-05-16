#include "Utils.h"

size_t MakeSize(size_t count)
{
    return make_aligned_float_element_count(count);
}

unique_aligned_buffer<float> MakeFloats(size_t count)
{
    auto floats = make_unique_buffer_aligned_to_vectorization_word<float>(count);

    size_t j = 0;

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats[j++] = static_cast<float>(i);
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats[j++] = (static_cast<float>(i) / 1000000.0f);
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats[j++] = (static_cast<float>(i) / 0.000001f);
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats[j++] = (25.0f / (static_cast<float>(i) + 1));
    }

    return floats;
}

unique_aligned_buffer<float> MakeFloats(size_t count, float value)
{
    auto floats = make_unique_buffer_aligned_to_vectorization_word<float>(count);

    for (size_t i = 0; i < count; ++i)
    {
        floats[i] = value;
    }

    return floats;
}

unique_aligned_buffer<ElementIndex> MakeElementIndices(ElementIndex maxElementIndex, size_t count)
{
    auto elementIndices = make_unique_buffer_aligned_to_vectorization_word<ElementIndex>(count);

    for (size_t i = 0; i < count; ++i)
    {
        elementIndices[i] = static_cast<ElementIndex>(i) % maxElementIndex;
    }

    return elementIndices;
}

unique_aligned_buffer<vec2f> MakeVectors(size_t count)
{
    auto vectors = make_unique_buffer_aligned_to_vectorization_word<vec2f>(count);

    for (size_t i = 0; i < count; ++i)
    {
        vectors[i] = vec2f(static_cast<float>(i), static_cast<float>(i) / 5.0f);
    }

    return vectors;
}