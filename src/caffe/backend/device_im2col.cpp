#include "caffe/backend/device.hpp"
#include "caffe/backend/device_program.hpp"
#include "caffe/backend/device_kernel.hpp"

namespace caffe {

template<typename Dtype>
string create_source(Device* dev,
                           shared_ptr<DeviceProgram> program) {
  stringstream ss;
  ss << program->define_type<Dtype>("Dtype");

  {
    KernelArgs fw_args;
    KernelArgs bw_args;

    // Forward im2col
    fw_args.push_back(program->create_kernel_arg<int_tp>("n",
                      KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<Dtype>("data_im",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    fw_args.push_back(program->create_kernel_arg<int_tp>("height",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("width",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("kernel_h",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("kernel_w",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("pad_h",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("pad_w",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("stride_h",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("stride_w",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("dilation_h",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("dilation_w",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("height_col",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("width_col",
                                                          KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<Dtype>("data_col",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_MEM_OFFSET));
    ss << program->function("im2col", fw_args);
    ss << program->kernel_loop("int_tp", "index", "n");
    ss << "const int_tp h_index = index / width_col;" << std::endl;
    ss << "const int_tp h_col = h_index % height_col;" << std::endl;
    ss << "const int_tp w_col = index % width_col;" << std::endl;
    ss << "const int_tp c_im = h_index / height_col;" << std::endl;
    ss << "const int_tp c_col = c_im * kernel_h * kernel_w;" << std::endl;
    ss << "const int_tp h_offset = h_col * stride_h - pad_h;" << std::endl;
    ss << "const int_tp w_offset = w_col * stride_w - pad_w;" << std::endl;
    ss << program->global_ptr("Dtype", "data_col_ptr")
       << " = data_col;" << std::endl;
    ss << "data_col_ptr += (c_col * height_col + h_col) * width_col + w_col";
    ss << program->global_ptr("const Dtype", "data_im_ptr")
       << " = data_im;" << std::endl;
    ss << "data_im_ptr += (c_im * height + h_offset) * width + w_offset;"
       << std::endl;
    ss << "for (int_tp i = 0; i < kernel_h; ++i) {" << std::endl;
    ss << "for (int_tp j = 0; j < kernel_w; ++j) {" << std::endl;
    ss << "int_tp h_im = h_offset + i * dilation_h;" << std::endl;
    ss << "int_tp w_im = w_offset + j * dilation_w;" << std::endl;
    ss << "*data_col_ptr = "
       << "(h_im >= 0 && w_im >= 0 && h_im < height && w_im < width) ?"
       << "data_im_ptr[i * dilation_h * width + j * dilation_w] : 0;"
       << std::endl;
    ss << "data_col_ptr += height_col * width_col;" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;

    // Backward col2im
    bw_args.push_back(program->create_kernel_arg<int_tp>("n",
                      KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<Dtype>("data_col",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    bw_args.push_back(program->create_kernel_arg<int_tp>("height",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("width",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("channels",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("kernel_h",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("kernel_w",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("pad_h",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("pad_w",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("stride_h",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("stride_w",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("dilation_h",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("dilation_w",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("height_col",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("width_col",
                                                          KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<Dtype>("data_im",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_MEM_OFFSET));
    ss << program->function("col2im", bw_args);
    ss << program->kernel_loop("int_tp", "index", "n");
    ss << "Dtype val = 0;" << std::endl;
    ss << "const int_tp w_im = index % width + pad_w;" << std::endl;
    ss << "const int_tp h_im = (index / width) % height + pad_h;" << std::endl;
    ss << "const int_tp c_im = index / (width * height);" << std::endl;
    ss << "int_tp kernel_extent_w = (kernel_w - 1) * dilation_w + 1;"
       << std::endl;
    ss << "int_tp kernel_extent_h = (kernel_h - 1) * dilation_h + 1;"
       << std::endl;
    // compute the start and end of the output
    ss << "const int_tp w_col_start = "
       << "(w_im < kernel_extent_w) ? 0 : (w_im - kernel_extent_w) "
       << "/ stride_w + 1;" << std::endl;
    ss << "const int_tp w_col_end = min(w_im / stride_w + 1, width_col);"
       << std::endl;
    ss << "const int_tp h_col_start = "
       << "(h_im < kernel_extent_h) ? 0 : (h_im - kernel_extent_h) "
       << "/ stride_h + 1;" << std::endl;
    ss << "const int_tp h_col_end = min(h_im / stride_h + 1, height_col);"
       << std::endl;
    // TODO: use LCM of stride and dilation to avoid unnecessary loops
    ss << "for (int_tp h_col = h_col_start; h_col < h_col_end; h_col += 1) {"
       << std::endl;
    ss << "for (int_tp w_col = w_col_start; w_col < w_col_end; w_col += 1) {"
       << std::endl;
    ss << "int_tp h_k = (h_im - h_col * stride_h);" << std::endl;
    ss << "int_tp w_k = (w_im - w_col * stride_w);" << std::endl;
    ss << "if (h_k % dilation_h == 0 && w_k % dilation_w == 0) {" << std::endl;
    ss << "h_k /= dilation_h;" << std::endl;
    ss << "w_k /= dilation_w;" << std::endl;
    ss << "int_tp data_col_index = (((c_im * kernel_h + h_k) * kernel_w + w_k) "
       << "* height_col + h_col) * width_col + w_col;" << std::endl;
    ss << "val += data_col[data_col_index];" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "data_im[index] = val;" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
  }

  for (int_tp num_axes = 0; num_axes < 6; ++num_axes) {
    KernelArgs fw_args;
    KernelArgs bw_args;

    // Forward im2col_nd
    fw_args.push_back(program->create_kernel_arg<int_tp>("n",
                      KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<Dtype>("data_im",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    fw_args.push_back(program->create_kernel_arg<int_tp>("im_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    fw_args.push_back(program->create_kernel_arg<int_tp>("col_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    fw_args.push_back(program->create_kernel_arg<int_tp>("kernel_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("pad",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("stride",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<int_tp>("dilation",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    fw_args.push_back(program->create_kernel_arg<Dtype>("data_col",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_MEM_OFFSET));
    ss << program->function("im2col_nd_" + std::to_string(num_axes), fw_args);
    ss << "int_tp d_temp[" << num_axes << "];" << std::endl;
    ss << "int_tp d_iter[" << num_axes << "];" << std::endl;

    ss << program->local_mem("int_tp") << "shared_dilation["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_kernel_shape["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_pad["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_stride["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_col_shape["
       << (num_axes + 1) << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_im_shape["
       << (num_axes + 1) << "];" << std::endl;

    ss << "for(int_tp li = " << program->local_id(0) << "; li < num_axes; "
       << "li += " << program->local_size(0) << ") {" << std::endl;
    ss << "shared_dilation[li] = dilation[li];" << std::endl;
    ss << "shared_kernel_shape[li] = kernel_shape[li];" << std::endl;
    ss << "shared_pad[li] = pad[li];" << std::endl;
    ss << "shared_stride[li] = stride[li];" << std::endl;
    ss << "}" << std::endl;
    ss << "for(int_tp li = " << program->local_id(0) << "; li < num_axes; "
       << "li += (" << program->local_size(0) << " + 1) {" << std::endl;
    ss << "shared_col_shape[li] = col_shape[li];" << std::endl;
    ss << "shared_im_shape[li] = im_shape[li];" << std::endl;
    ss << "}" << std::endl;

    ss << program->local_barrier() << std::endl;

    ss << "int_tp i;" << std::endl;
    ss << program->kernel_loop("int_tp", "index", "n") << std::endl;
    // Initialize channel_in, computed in the loop below, with intermediate
    // computations used to compute the spatial indices.
    ss << "int_tp channel_in = index;" << std::endl;
    ss << "int_tp channel_out = 1;" << std::endl;
    ss << "for (i = num_axes - 1; i >= 0; --i) {" << std::endl;
    ss << "d_temp[i] = channel_in % shared_col_shape[i + 1];" << std::endl;
    ss << "channel_in /= shared_col_shape[i + 1];" << std::endl;
    ss << "channel_out *= shared_kernel_shape[i];" << std::endl;
    ss << "}" << std::endl;
    ss << "channel_out *= channel_in;" << std::endl;
    ss << "int_tp data_col_inc = 1;" << std::endl;
    ss << "for (i = 0; i < num_axes; ++i) {" << std::endl;
    ss << "channel_out *= shared_col_shape[i + 1];" << std::endl;
    ss << "channel_out += d_temp[i];" << std::endl;
    ss << "d_temp[i] = d_temp[i] * shared_stride[i] - shared_pad[i];"
       << std::endl;
    ss << "channel_in *= shared_im_shape[i + 1];" << std::endl;
    ss << "channel_in += d_temp[i];" << std::endl;
    ss << "data_col_inc *= shared_col_shape[i + 1];" << std::endl;
    ss << "d_iter[i] = 0;" << std::endl;
    ss << "}" << std::endl;
    ss << program->global_ptr("Dtype", "data_col_ptr")
       << " = data_col + channel_out;" << std::endl;
    ss << program->global_ptr("const Dtype", "data_im_ptr")
       << " = data_im + channel_in;" << std::endl;
    ss << "bool incremented;" << std::endl;
    ss << "do {" << std::endl;
    ss << "bool in_range = true;" << std::endl;
    ss << "for (i = 0; i < num_axes; ++i) {" << std::endl;
    ss << "const int_tp d_iter_im = d_iter[i] * shared_dilation[i] + d_temp[i];"
       << std::endl;
    ss << "in_range &= d_iter_im >= 0 && d_iter_im < shared_im_shape[i + 1];"
       << std::endl;
    ss << "if (!in_range) { break; }" << std::endl;
    ss << "}" << std::endl;
    ss << "if (in_range) {" << std::endl;
    ss << "int_tp data_im_offset = d_iter[0] * shared_dilation[0];"
       << std::endl;
    ss << "for (i = 1; i < num_axes; ++i) {" << std::endl;
    ss << "data_im_offset *= shared_im_shape[i + 1];" << std::endl;
    ss << "data_im_offset += d_iter[i] * shared_dilation[i];" << std::endl;
    ss << "}" << std::endl;
    ss << "*data_col_ptr = data_im_ptr[data_im_offset];" << std::endl;
    ss << "} else {" << std::endl;
    ss << "*data_col_ptr = 0;" << std::endl;
    ss << "}" << std::endl;
    ss << "data_col_ptr += data_col_inc;" << std::endl;
    ss << "incremented = false;" << std::endl;
    ss << "for (i = num_axes - 1; i >= 0; --i) {" << std::endl;
    ss << "const int_tp d_max = shared_kernel_shape[i];" << std::endl;
    ss << "if (d_iter[i] == d_max - 1) {" << std::endl;
    ss << "d_iter[i] = 0;" << std::endl;
    ss << "} else {" << std::endl;  // d_iter[i] < d_max - 1
    ss << "++d_iter[i];" << std::endl;
    ss << "incremented = true;" << std::endl;
    ss << "break;" << std::endl;
    ss << "}" << std::endl;
    ss << "}  // for (int_tp i = num_axes - 1; i >= 0; --i)" << std::endl;
    ss << "} while (incremented);" << std::endl;  // do
    ss << "}" << std::endl;  // KERNEL_LOOP(index, n)
    ss << "}" << std::endl;

    // Backward col2im_nd
    bw_args.push_back(program->create_kernel_arg<int_tp>("n",
                      KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<Dtype>("data_col",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    bw_args.push_back(program->create_kernel_arg<int_tp>("im_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    bw_args.push_back(program->create_kernel_arg<int_tp>("col_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST |
                      KERNEL_ARG_MEM_OFFSET));
    bw_args.push_back(program->create_kernel_arg<int_tp>("kernel_shape",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("pad",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("stride",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<int_tp>("dilation",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_CONST));
    bw_args.push_back(program->create_kernel_arg<Dtype>("data_im",
                      KERNEL_ARG_GLOBAL_MEM | KERNEL_ARG_MEM_OFFSET));
    ss << program->function("col2im_nd_" + std::to_string(num_axes), bw_args);
    ss << "int_tp d_im[" << num_axes << "];" << std::endl;
    ss << "int_tp d_col_iter[" << num_axes << "];" << std::endl;
    ss << "int_tp d_col_start[" << num_axes << "];" << std::endl;
    ss << "int_tp d_col_end[" << num_axes << "];" << std::endl;

    ss << program->local_mem("int_tp") << "shared_dilation["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_kernel_shape["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_pad["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_stride["
       << num_axes << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_col_shape["
       << (num_axes + 1) << "];" << std::endl;
    ss << program->local_mem("int_tp") << "shared_im_shape["
       << (num_axes + 1) << "];" << std::endl;

    ss << "for(int_tp li = " << program->local_id(0) << "; li < num_axes; "
       << "li += " << program->local_size(0) << ") {" << std::endl;
    ss << "shared_dilation[li] = dilation[li];" << std::endl;
    ss << "shared_kernel_shape[li] = kernel_shape[li];" << std::endl;
    ss << "shared_pad[li] = pad[li];" << std::endl;
    ss << "shared_stride[li] = stride[li];" << std::endl;
    ss << "}" << std::endl;
    ss << "for(int_tp li = " << program->local_id(0) << "; li < num_axes; "
       << "li += (" << program->local_size(0) << " + 1) {" << std::endl;
    ss << "shared_col_shape[li] = col_shape[li];" << std::endl;
    ss << "shared_im_shape[li] = im_shape[li];" << std::endl;
    ss << "}" << std::endl;

    ss << program->local_barrier() << std::endl;

    ss << program->kernel_loop("int_tp", "index", "n") << std::endl;
    // Initialize channel_in, computed in the loop below, with int_tpermediate
    // computations used to compute the spatial indices.
    ss << "int_tp c_im = index;" << std::endl;
    // Calculate d_im (image dimensions).
    ss << "for (int_tp i = num_axes - 1; i >= 0; --i) {" << std::endl;
    ss << "d_im[i] = c_im % shared_im_shape[i + 1] + shared_pad[i];"
       << std::endl;
    ss << "c_im /= shared_im_shape[i + 1];" << std::endl;
    ss << "}" << std::endl;
    // Calculate col start/end indices.
    ss << "bool done = false;" << std::endl;
    ss << "for (int_tp i = 0; i < num_axes; ++i) {" << std::endl;
    ss << "const int_tp kernel_extent = "
       << "shared_dilation[i] * (shared_kernel_shape[i] - 1) + 1;" << std::endl;
    ss << "d_col_start[i] = d_col_iter[i] ="
       << "(d_im[i] < kernel_extent) ? 0 :"
       << "(d_im[i] - kernel_extent) / shared_stride[i] + 1;" << std::endl;
    ss << "d_col_end[i] ="
       << "min(d_im[i] / shared_stride[i] + 1, shared_col_shape[i + 1]);"
       << std::endl;
    ss << "if (d_col_start[i] >= d_col_end[i]) {" << std::endl;
    // Skip computation if the dimension is 0 at any spatial axis --
    // final val will be 0.
    ss << "data_im[index] = (Dtype)0.0;" << std::endl;
    ss << "done = true;" << std::endl;
    ss << "break;  // for (int_tp i = 0; i < num_axes; ++i)" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "if (done) {" << std::endl;
    ss << "continue;" << std::endl;  // CUDA_KERNEL_LOOP(index, n)
    ss << "}" << std::endl;
    // Loop over the col to compute the output val.
    ss << "Dtype val = (Dtype)0.0;" << std::endl;
    ss << "bool incremented = true;" << std::endl;
    ss << "bool skip = false;" << std::endl;
    ss << "do {" << std::endl;
    // Compute the final offset.
    ss << "int_tp final_offset = 0;" << std::endl;
    ss << "int_tp kernel_shape_prod = 1;" << std::endl;
    ss << "int_tp kernel_index;" << std::endl;
    ss << "for (int_tp i = num_axes - 1; i >= 0; --i) {" << std::endl;
    ss << "kernel_index = d_im[i] - d_col_iter[i] * shared_stride[i];"
       << std::endl;
    ss << "if (kernel_index % shared_dilation[i]) {" << std::endl;
    ss << "skip = true;" << std::endl;
    ss << "break;" << std::endl;
    ss << "} else {" << std::endl;
    ss << "kernel_index /= shared_dilation[i];" << std::endl;
    ss << "final_offset += kernel_index * kernel_shape_prod;" << std::endl;
    ss << "kernel_shape_prod *= shared_kernel_shape[i];" << std::endl;
    ss << "}" << std::endl;
    ss << "}" << std::endl;
    ss << "if (!skip) {" << std::endl;
    ss << "final_offset += kernel_shape_prod * c_im;" << std::endl;
    ss << "for (int_tp i = 0; i < num_axes; ++i) {" << std::endl;
    ss << "final_offset *= shared_col_shape[i + 1];" << std::endl;
    ss << "final_offset += d_col_iter[i];" << std::endl;
    ss << "}" << std::endl;
    ss << "val += data_col[final_offset];" << std::endl;
    ss << "}" << std::endl;
    ss << "skip = false;" << std::endl;
    ss << "incremented = false;" << std::endl;
    ss << "for (int_tp i = num_axes - 1; i >= 0; --i) {" << std::endl;
    ss << "const int_tp d_max = d_col_end[i];" << std::endl;
    ss << "if (d_col_iter[i] == d_max - 1) {" << std::endl;
    ss << "d_col_iter[i] = d_col_start[i];" << std::endl;
    ss << "} else {" << std::endl;  // d_col_iter[i] < d_max - 1
    ss << "++d_col_iter[i];" << std::endl;
    ss << "incremented = true;" << std::endl;
    ss << "break;" << std::endl;  // for (int_tp i = num_axes - 1; i >= 0; --i)
    ss << "}" << std::endl;
    ss << "}" << std::endl;  // for (int_tp i = num_axes - 1; i >= 0; --i)
    ss << "} while (incremented);" << std::endl;
    ss << "data_im[index] = val;" << std::endl;
    ss << "}" << std::endl;  // KERNEL_LOOP(index, n)
    ss << "}" << std::endl;
  }
  return ss.str();
}

template<>
string create_source<half_float::half>(Device* dev,
    shared_ptr<DeviceProgram> program);
template<>
string create_source<float>(Device* dev,
    shared_ptr<DeviceProgram> program);
template<>
string create_source<double>(Device* dev,
    shared_ptr<DeviceProgram> program);

void Device::CreateIm2ColProgram() {
  this->im2col_program_ = this->CreateProgram();
  stringstream ss;

  ss << this->im2col_program_->setup();

#ifdef USE_GPU_HALF
  ss << create_source<half_float::half>(this, this->math_program_);
#endif

#ifdef USE_GPU_SINGLE
  ss << create_source<float>(this, this->math_program_);
#endif

#ifdef USE_GPU_DOUBLE
  ss << create_source<double>(this, this->math_program_);
#endif

  this->im2col_program_->set_source(ss.str());
  this->im2col_program_->Compile(true, true);
}


template<typename Dtype>
void Device::im2col(vptr<const Dtype> data_im, const int_tp channels,
            const int_tp height, const int_tp width, const int_tp kernel_h,
            const int_tp kernel_w, const int_tp pad_h, const int_tp pad_w,
            const int_tp stride_h, const int_tp stride_w,
            const int_tp dilation_h, const int_tp dilation_w,
            vptr<Dtype> data_col) {
  // We are going to launch channels * height_col * width_col kernels, each
  // kernel responsible for copying a single-channel grid.
  int_tp height_col = (height + 2 * pad_h - (dilation_h * (kernel_h - 1) + 1))
      / stride_h + 1;
  int_tp width_col = (width + 2 * pad_w - (dilation_w * (kernel_w - 1) + 1))
      / stride_w + 1;
  int_tp num_kernels = channels * height_col * width_col;

  shared_ptr<DeviceKernel> kernel = this->im2col_program_->GetKernel("im2col");
  kernel->add_arg(&num_kernels);
  kernel->add_arg(&data_im);
  kernel->add_arg(&height);
  kernel->add_arg(&width);
  kernel->add_arg(&kernel_h);
  kernel->add_arg(&kernel_w);
  kernel->add_arg(&pad_h);
  kernel->add_arg(&pad_w);
  kernel->add_arg(&stride_h);
  kernel->add_arg(&stride_w);
  kernel->add_arg(&dilation_h);
  kernel->add_arg(&dilation_w);
  kernel->add_arg(&height_col);
  kernel->add_arg(&width_col);
  kernel->add_arg(&data_col);

  vector<size_t> work_size(1, num_kernels);
  vector<size_t> group;
  vector<size_t> local;
  this->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

template<typename Dtype>
void Device::col2im(vptr<const Dtype> data_col, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
            vptr<Dtype> data_im) {
  int_tp height_col = (height + 2 * pad_h - (dilation_h * (kernel_h - 1) + 1)) /
      stride_h + 1;
  int_tp width_col = (width + 2 * pad_w - (dilation_w * (kernel_w - 1) + 1)) /
      stride_w + 1;
  int_tp num_kernels = channels * height * width;
  // To avoid involving atomic operations, we will launch one kernel per
  // bottom dimension, and then in the kernel add up the top dimensions.

  shared_ptr<DeviceKernel> kernel = this->im2col_program_->GetKernel("col2im");
  kernel->add_arg(&num_kernels);
  kernel->add_arg(&data_col);
  kernel->add_arg(&height);
  kernel->add_arg(&width);
  kernel->add_arg(&channels);
  kernel->add_arg(&kernel_h);
  kernel->add_arg(&kernel_w);
  kernel->add_arg(&pad_h);
  kernel->add_arg(&pad_w);
  kernel->add_arg(&stride_h);
  kernel->add_arg(&stride_w);
  kernel->add_arg(&dilation_h);
  kernel->add_arg(&dilation_w);
  kernel->add_arg(&height_col);
  kernel->add_arg(&width_col);
  kernel->add_arg(&data_im);

  vector<size_t> work_size(1, num_kernels);
  vector<size_t> group;
  vector<size_t> local;
  this->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

template<typename Dtype>
void Device::im2col_nd(vptr<const Dtype> data_im, const int_tp num_spatial_axes,
                       const int_tp num_kernels, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape, vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<Dtype> data_col) {
  shared_ptr<DeviceKernel> kernel = this->im2col_program_
                                                       ->GetKernel("im2col_nd");
  kernel->add_arg(&num_kernels);
  kernel->add_arg(&data_im);
  kernel->add_arg(&im_shape);
  kernel->add_arg(&col_shape);
  kernel->add_arg(&kernel_shape);
  kernel->add_arg(&pad);
  kernel->add_arg(&stride);
  kernel->add_arg(&dilation);
  kernel->add_arg(&data_col);

  vector<size_t> work_size(1, num_kernels);
  vector<size_t> group;
  vector<size_t> local;
  this->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

template<typename Dtype>
void Device::col2im_nd(vptr<const Dtype> data_col,
                       const int_tp num_spatial_axes,
                       const int_tp im_size, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<Dtype> data_im) {
  shared_ptr<DeviceKernel> kernel = this->im2col_program_
                                                       ->GetKernel("col2im_nd");
  kernel->add_arg(&im_size);
  kernel->add_arg(&data_col);
  kernel->add_arg(&im_shape);
  kernel->add_arg(&col_shape);
  kernel->add_arg(&kernel_shape);
  kernel->add_arg(&pad);
  kernel->add_arg(&stride);
  kernel->add_arg(&dilation);
  kernel->add_arg(&data_im);

  vector<size_t> work_size(1, im_size);
  vector<size_t> group;
  vector<size_t> local;
  this->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

#ifdef USE_GPU_HALF
template<>
void Device::im2col(vptr<const half_float::half> data_im, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
                    vptr<half_float::half> data_col);
template<>
void Device::col2im(vptr<const half_float::half> data_col,
                    const int_tp channels, const int_tp height,
                    const int_tp width, const int_tp kernel_h,
                    const int_tp kernel_w, const int_tp pad_h,
                    const int_tp pad_w, const int_tp stride_h,
                    const int_tp stride_w, const int_tp dilation_h,
                    const int_tp dilation_w, vptr<half_float::half> data_im);
template<>
void Device::im2col_nd(vptr<const half_float::half> data_im,
                       const int_tp num_spatial_axes,
                       const int_tp num_kernels, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation,
                       vptr<half_float::half> data_col);
template<>
void Device::col2im_nd(vptr<const half_float::half> data_col,
                       const int_tp num_spatial_axes,
                       const int_tp im_size, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation,
                       vptr<half_float::half> data_im);
#endif  // USE_GPU_HALF


#ifdef USE_GPU_SINGLE
template<>
void Device::im2col(vptr<const float> data_im, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
                    vptr<float> data_col);
template<>
void Device::col2im(vptr<const float> data_col, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
                    vptr<float> data_im);
template<>
void Device::im2col_nd(vptr<const float> data_im,
                       const int_tp num_spatial_axes,
                       const int_tp num_kernels, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<float> data_col);
template<>
void Device::col2im_nd(vptr<const float> data_col,
                       const int_tp num_spatial_axes,
                       const int_tp im_size, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<float> data_im);
#endif  // USE_GPU_SINGLE


#ifdef USE_GPU_DOUBLE
template<>
void Device::im2col(vptr<const double> data_im, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
                    vptr<double> data_col);
template<>
void Device::col2im(vptr<const double> data_col, const int_tp channels,
                    const int_tp height, const int_tp width,
                    const int_tp kernel_h, const int_tp kernel_w,
                    const int_tp pad_h, const int_tp pad_w,
                    const int_tp stride_h, const int_tp stride_w,
                    const int_tp dilation_h, const int_tp dilation_w,
                    vptr<double> data_im);
template<>
void Device::im2col_nd(vptr<const double> data_im,
                       const int_tp num_spatial_axes,
                       const int_tp num_kernels, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<double> data_col);
template<>
void Device::col2im_nd(vptr<const double> data_col,
                       const int_tp num_spatial_axes,
                       const int_tp im_size, vptr<const int_tp> im_shape,
                       vptr<const int_tp> col_shape,
                       vptr<const int_tp> kernel_shape,
                       vptr<const int_tp> pad, vptr<const int_tp> stride,
                       vptr<const int_tp> dilation, vptr<double> data_im);
#endif  // USE_GPU_DOUBLE

}  // namespace caffe
