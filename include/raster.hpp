// Copyright (C) 2020-2022 by Rob Menke.  All rights reserved.  See
// accompanying LICENSE.txt file for details.

#ifndef ppht_raster_hpp
#define ppht_raster_hpp

#include <memory>

namespace ppht {

/**
 * @brief A simple class to manage a 2D array of elements.
 *
 * Does not perform bounds checking or row alignment.
 *
 * @tparam T the type of elements to store.
 */
template <class T>
class raster {
    /// The elements of the raster.
    std::unique_ptr<T[]> _data;

    /// The height of this raster.
    std::size_t const _rows;

    /// The width of this raster.
    std::size_t const _cols;

  public:
    /// The type of the cells of this raster.
    using value_type = T;

    /**
     * @brief Create a new raster with the given size.
     *
     * @param rows the number of rows (height) of the raster.
     *
     * @param cols the number of columns (width) of the raster.
     */
    raster(std::size_t rows, std::size_t cols)
        : _data(new T[rows * cols]{})
        , _rows(rows)
        , _cols(cols) {}

    /**
     * @brief Get the height of the raster.
     *
     * @return the number of rows in the raster
     */
    std::size_t const &rows() const {
        return _rows;
    }

    /**
     * @brief Get the width of the raster.
     *
     * @return the number of columns in the raster
     */
    std::size_t const &cols() const {
        return _cols;
    }

    /**
     * @brief Access the specified row of the raster.
     *
     * The returned value will be a pointer to an array of length @ref
     * cols made of @ref value_type elements.  This method does not do
     * bounds checking.
     *
     * @param row the row number
     *
     * @return a pointer to the row
     */
    T *operator[](std::size_t row) {
        return _data.get() + row * _cols;
    }

    /**
     * @brief Access the specified row of the raster as a read-only
     * array.
     *
     * The returned value will be a pointer to an array of length @ref
     * cols made of @ref value_type elements.  This method does not do
     * bounds checking.
     *
     * @param row the row number
     *
     * @return a pointer to the row
     */
    T const *operator[](std::size_t row) const {
        return _data.get() + row * _cols;
    }
};

} // namespace ppht

#endif /* ppht_raster_hpp */
