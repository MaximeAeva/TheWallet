#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

class Matrix
{
    Matrix(size_t rows, size_t cols);
        double& operator()(size_t i, size_t j);
        double operator()(size_t i, size_t j) const;

    private:
        size_t mRows;
        size_t mCols;
        std::vector<double> mData;
};

#endif // MATRIX_H
