#include <ATen/ATen.h>
#include <ATen/NativeFunctions.h>
#include <ATen/native/AdaptivePooling.h>


namespace at {
namespace meta {
TORCH_META_FUNC(adaptive_max_pool2d) (const Tensor& input, IntArrayRef output_size) {
  for (int64_t i = 0; i < input.ndimension(); i++) {
    TORCH_CHECK(
        input.size(i) > 0,
        "adaptive_max_pool2d: expected input to have non-empty spatial dimensions, "
        "but input has sizes ",
        input.sizes(),
        " with dimension ",
        i,
        " being "
        "empty");
  }

  TORCH_CHECK(
      (input.ndimension() == 3 || input.ndimension() == 4),
      "non-empty 3D or 4D (batch mode) tensor expected for input");

  TORCH_CHECK(
      output_size.size() == 2,
      "adaptive_max_pool2d: internal error: output_size.size() must be 2");

  int dimH = 1;
  int64_t sizeB = 1;
  int64_t sizeD = 0;

  if (input.ndimension() == 4) {
    sizeB = input.size(0);
    dimH++;
  }

  sizeD = input.size(dimH - 1);

  int64_t osizeH = output_size[0];
  int64_t osizeW = output_size[1];

  /* resize output */
  if (input.ndimension() == 3) {
    set_output(0, {sizeD, osizeH, osizeW}, input.options());
    /* indices will contain i,j locations for each output point */
    set_output(1, {sizeD, osizeH, osizeW}, input.options().dtype(kLong));
  } else {
    set_output(0, {sizeB, sizeD, osizeH, osizeW}, input.options().memory_format(input.suggest_memory_format()));
    /* indices will contain i,j locations for each output point */
    set_output(1, {sizeB, sizeD, osizeH, osizeW}, input.options().memory_format(input.suggest_memory_format()).dtype(kLong));
  }
}
} // namespace meta

namespace native {

namespace {

Tensor& adaptive_max_pool2d_backward_out_cpu_template(
          Tensor& grad_input,
          const Tensor& grad_output,
          const Tensor& input,
          const Tensor& indices)
{
  int64_t ndim = grad_output.ndimension();
  for (int64_t i = 0; i < ndim; i++) {
    TORCH_CHECK(grad_output.size(i) > 0,
      "adaptive_max_pooling2d_backward(): expected grad_output to have non-empty spatial dimensions, "
      "but grad_output has sizes ", grad_output.sizes(), " with dimension ", i, " being "
      "empty");
  }

  TORCH_CHECK((ndim == 3 || ndim == 4),
    "non-empty 3D or 4D (batch mode) tensor expected for grad_output");
  TORCH_CHECK(input.dtype() == grad_output.dtype(),
    "expected dtype ", input.dtype(), " for `grad_output` but got dtype ", grad_output.dtype());
  TORCH_CHECK(input.dtype() == grad_input.dtype(),
    "expected dtype ", input.dtype(), " for `grad_input` but got dtype ", grad_input.dtype());

  grad_input.resize_(input.sizes(), input.suggest_memory_format());
  grad_input.zero_();

  adaptive_max_pool2d_backward_kernel(kCPU, grad_input, grad_output, indices);
  return grad_input;
}

} // namespace

TORCH_IMPL_FUNC(adaptive_max_pool2d_out_cpu)
(const Tensor& input, IntArrayRef output_size, const Tensor& output, const Tensor& indices) {
  adaptive_max_pool2d_kernel(kCPU, output, indices, input, output_size);
}

Tensor& adaptive_max_pool2d_backward_out_cpu(
  const Tensor& grad_output,
  const Tensor& input,
  const Tensor& indices,
  Tensor& grad_input)
{
  adaptive_max_pool2d_backward_out_cpu_template(
    grad_input,
    grad_output,
    input,
    indices);
  return grad_input;
}

Tensor adaptive_max_pool2d_backward_cpu(
  const Tensor& grad_output,
  const Tensor& input,
  const Tensor& indices)
{
  auto grad_input = at::empty({0}, input.options());
  adaptive_max_pool2d_backward_out_cpu_template(
    grad_input,
    grad_output,
    input,
    indices);
  return grad_input;
}

DEFINE_DISPATCH(adaptive_max_pool2d_kernel);
DEFINE_DISPATCH(adaptive_max_pool2d_backward_kernel);

} // at::native
} // at
