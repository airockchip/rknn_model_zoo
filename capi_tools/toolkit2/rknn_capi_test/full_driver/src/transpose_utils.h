/*!
 * \brief Count elements in a tensor
 *
 * This function returns count of elements given shape of a tensor.
 */
template <typename TensorIndexType>
static TensorIndexType tensor_size(const TensorIndexType* shape, TensorIndexType order)
{
  TensorIndexType result = 1;

  for (TensorIndexType i = 0; i < order; ++i)
    result *= shape[i];

  return result;
}


#ifndef restrict
#define restrict
#endif

/*!
 * \brief Inner product of vectors of indices
 *
 * This function computes data offset from a multidimensional index.
 * See tensor_strides() for details.
 */
template <typename TensorIndexType>
static TensorIndexType tensor_idot(const TensorIndexType* x, const TensorIndexType* y, TensorIndexType count)
{
  typedef TensorIndexType Index;
  Index                   result = 0;

  for (Index i = 0; i < count; ++i)
    result += x[i] * y[i];

  return result;
}

/*!
 * \brief Increment 0-based multidimensional index in a tensor
 *
 * For example, in a 2x1x3 tensor, an index shall iterate through
 *
 *     { 0, 0, 0 }
 *     { 0, 0, 1 }
 *     { 0, 0, 2 }
 *     { 1, 0, 0 }
 *     { 1, 0, 1 }
 *     { 1, 0, 2 }
 */
template <typename TensorIndexType>
static void tensor_increment(TensorIndexType* restrict index, const TensorIndexType* restrict shape,
                             TensorIndexType order)
{
  typedef TensorIndexType Index;
  Index                   end = order - 1;

  if (order > 0 && ++index[end] >= shape[end]) {
    index[end] = 0;
    tensor_increment<TensorIndexType>(index, shape, end);
  }
}

/*!
 * \brief Strides for each axis
 *
 * This function computes strides given shape of a tensor.
 *
 * For example, given a 3x1x3x3x7 tensor, its strides on each axis are
 *
 *     { 63, 0, 21, 7, 1 }
 *
 * In this case, we can access the element at `[a, b, c, d, e]` with
 *
 *     pointer[63*a + 21*c + 7*d + e]
 *
 * The stride on each 1-element-long axis is thought to be 0 to ease arbitrary
 * broadcasting.  For example, the example array of strides still works if the
 * tensor is read as a 3xnx3x3x7 tensor.
 */
template <typename TensorIndexType>
static void tensor_strides(TensorIndexType* restrict result, const TensorIndexType* restrict shape,
                           TensorIndexType order)
{
  typedef TensorIndexType Index;

  if (order > 0)
    result[order - 1] = 1;

  for (Index i = order - 1; i > 0; --i)
    result[i - 1] = shape[i] * result[i];

  for (Index i = 0; i < order; ++i)
    result[i] *= shape[i] > 1;
}


/*!
 * \brief Permutation of index vector
 *
 * The permutation specifier is the same as described by ONNX `Transpose`
 * operator, which is an array of indices.  For example, when
 * `permutation = { 1, 0, 2 }` and `index = { a, b, c }`, this function writes
 * the permuted index `{ b, a, c }` to `result`.
 */
template <typename TensorIndexType>
static void tensor_permute(TensorIndexType* restrict result, const TensorIndexType* restrict permutation,
                           const TensorIndexType* restrict index, TensorIndexType order)
{
  for (TensorIndexType i = 0; i < order; ++i)
    result[i] = index[permutation[i]];
}

/*!
 * \brief Tensor transpose
 *
 * This is generalization of matrix transpose to multidimensional space.
 * As there is more than one non-trivial transpose in 3+ ordered tensors,
 * transpose is described as permutation of indices.  See tensor_permute() for
 * details of index permutation.
 */
#define TENSOR_TRANSPOSE(SCALAR, INDEX, y, x, permutation, shape, order) \
  do {                                                                   \
    typedef SCALAR Scalar;                                               \
    typedef INDEX  Index;                                                \
                                                                         \
    Scalar* restrict       _y           = y;                             \
    const Scalar* restrict _x           = x;                             \
    const Index* restrict  _permutation = permutation;                   \
    const Index* restrict  _shape       = shape;                         \
    Index                  _order       = order;                         \
                                                                         \
    Index _size = tensor_size<INDEX>(_shape, _order);                    \
    Index _index[_order], _permuted[_order], _strides[_order];           \
                                                                         \
    for (Index _i = 0; _i < _order; ++_i)                                \
      _index[_i] = 0;                                                    \
                                                                         \
    tensor_strides<INDEX>(_permuted, _shape, _order);                    \
    tensor_permute<INDEX>(_strides, _permutation, _permuted, _order);    \
    tensor_permute<INDEX>(_permuted, _permutation, _shape, _order);      \
                                                                         \
    for (Index _i = 0; _i < _size; ++_i) {                               \
      _y[_i] = _x[tensor_idot<INDEX>(_index, _strides, _order)];         \
      tensor_increment<INDEX>(_index, _permuted, _order);                \
    }                                                                    \
  } while (0)

