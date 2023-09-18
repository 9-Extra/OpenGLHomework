#pragma once
#include "../utils/CGmath.h"

struct Transform{
    Vector3f position;
    Vector3f rotation;
    Vector3f scale;

    Matrix transform_matrix() const{
        return Matrix::translate(position) * Matrix::rotate(rotation) * Matrix::scale(scale);
    }
};