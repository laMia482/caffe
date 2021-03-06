#include <boost/filesystem.hpp>

#include "caffe/backend/opencl/ocl_device_program.hpp"
#include "caffe/backend/backend.hpp"
#include "caffe/backend/device.hpp"

namespace caffe {

#ifdef USE_OPENCL

OclDeviceProgram::OclDeviceProgram(Device *dev) : DeviceProgram(dev) {
}

void OclDeviceProgram::Compile(bool load_cache, bool store_cache) {
  viennacl::ocl::context &ctx = viennacl::ocl::get_context(
      this->device_->id());

  string build_opts = "";

  build_opts += "-cl-fast-relaxed-math -cl-mad-enable ";

  //    build_opts += "-cl-single-precision-constant ";

  ctx.build_options(build_opts);

  ocl_program_ = ctx.add_program(src_.c_str(), "kernel_program");

#ifndef NDEBUG
  string debug_path = ".caffe_debug";
  const char* path = debug_path.c_str();
  boost::filesystem::path dir(path);
  boost::filesystem::create_directory(dir);

  size_t bin_sz;
  clGetProgramInfo(ocl_program_.handle().get(),
                   CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &bin_sz, NULL);
  unsigned char *bin = (unsigned char *)malloc(bin_sz);  // NOLINT
  clGetProgramInfo(ocl_program_.handle().get(),
                   CL_PROGRAM_BINARIES, sizeof(unsigned char *), &bin, NULL);
  FILE* fp = fopen((".caffe_debug/" + string_identifier() + ".clptx").c_str(),
                   "wb");
  fwrite(bin, sizeof(char), bin_sz, fp);
  fclose(fp);
  free(bin);  // NOLINT
#endif  // NDEBUG
}

shared_ptr<DeviceKernel> OclDeviceProgram::GetKernel(string name) {
  viennacl::ocl::kernel &kernel = this->ocl_program_.get_kernel(name);

  KernelArgs args;

  std::map<string, KernelArgs>::iterator pos = this->args_.find(name);
  if (pos == this->args_.end()) {
    LOG(FATAL) << "OpenCL kernel " << name << " not found";
  } else {
    args = pos->second;
  }

  return std::make_shared<OclDeviceKernel>(device_, kernel, args);
}

string OclDeviceProgram::function(string name, KernelArgs args) {
  args_.insert(make_pair(name, args));
  stringstream ss;
  ss << "__kernel void ";
  ss << name << "(";
  for (uint_tp i = 0; i < args.size(); ++i) {
    uint64_t flags = std::get<2>(args[i]);
    if ((flags & KERNEL_ARG_GLOBAL_MEM) == KERNEL_ARG_GLOBAL_MEM) {
      ss << "__global ";
    }
    if ((flags & KERNEL_ARG_LOCAL_MEM) == KERNEL_ARG_LOCAL_MEM) {
      ss << "__local ";
    }
    if ((flags & KERNEL_ARG_CONST) == KERNEL_ARG_CONST) {
      ss << "const ";
    }
    string var_name = std::get<1>(args[i]);
    string off_name = std::get<1>(args[i]);
    if ((flags & KERNEL_ARG_MEM_OFFSET) == KERNEL_ARG_MEM_OFFSET) {
      var_name += "_raw_ptr";
      off_name += "_offset";
    }
    ss << std::get<0>(args[i]) << " " << var_name;
    if ((flags & KERNEL_ARG_MEM_OFFSET) == KERNEL_ARG_MEM_OFFSET) {
      ss << ", const uint_tp " << off_name;
    }
    if (i < args.size() - 1) {
      ss << ", ";
    }
  }
  ss << ") {" << std::endl;
  for (uint_tp i = 0; i < args.size(); ++i) {
    uint64_t flags = std::get<2>(args[i]);
    if ((flags & KERNEL_ARG_MEM_OFFSET) == KERNEL_ARG_MEM_OFFSET) {
      string base_name = std::get<1>(args[i]);
      string var_name = base_name + "_raw_ptr";
      string off_name = base_name + "_offset";
      if ((flags & KERNEL_ARG_GLOBAL_MEM) == KERNEL_ARG_GLOBAL_MEM) {
        ss << "__global ";
      }
      if ((flags & KERNEL_ARG_LOCAL_MEM) == KERNEL_ARG_LOCAL_MEM) {
        ss << "__local ";
      }
      if ((flags & KERNEL_ARG_CONST) == KERNEL_ARG_CONST) {
        ss << "const ";
      }
      ss << std::get<0>(args[i]) << " " << base_name;
      ss << " = " << var_name << " + " << off_name;
      ss << std::endl;
    }
  }
  return ss.str();
}

string OclDeviceProgram::kernel_loop(string type,
                                            string index, string n) {
  stringstream ss;
  ss << "for (" << type << " "
     << index << " = get_global_id(0); "
     << index << " < " << n << "; "
     << index << " += get_global_size(0)) {" << std::endl;
  return ss.str();
}

string OclDeviceProgram::setup() {
  stringstream ss;
  // Test/enable KHR 64 bit (double)
  ss << "#if defined(cl_khr_fp64)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_khr_fp64 : enable" << std::endl;
  ss << "#define DOUBLE_SUPPORT_AVAILABLE" << std::endl;

  // Test/enable AMD 64 bit (double)
  ss << "#elif defined(cl_amd_fp64)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_amd_fp64 : enable" << std::endl;
  ss << "#define DOUBLE_SUPPORT_AVAILABLE" << std::endl;
  ss << "#endif" << std::endl;

  // Test/enable KHR 16 bit (half)
  ss << "#if defined(cl_khr_fp16)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_khr_fp16 : enable" << std::endl;
  ss << "#define HALF_SUPPORT_AVAILABLE" << std::endl;
  ss << "#endif" << std::endl;

  ss << this->define_type<int_tp>("int_tp");
  ss << this->define_type<uint_tp>("uint_tp");
  ss << this->define_type<int_tp>("int_tpc");
  ss << this->define_type<uint_tp>("uint_tpc");
  return ss.str();
}

string OclDeviceProgram::global_ptr(string type, string name) {
  return "__global " + type + "* " + name;
}

string OclDeviceProgram::local_ptr(string type, string name) {
  return "__local " + type + "* " + name;
}

string OclDeviceProgram::local_mem(string type) {
  return "__local " + type;
}

string OclDeviceProgram::local_id(uint_tp fixed_index) {
  return "get_local_id(" + std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::local_id(string runtime_index) {
  return "get_local_id(" + runtime_index + ")";
}
string OclDeviceProgram::local_size(uint_tp fixed_index) {
  return "get_local_size(" + std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::local_size(string runtime_index) {
  return "get_local_size(" + runtime_index + ")";
}
string OclDeviceProgram::group_id(uint_tp fixed_index) {
  return "get_group_id(" + std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::group_id(string runtime_index) {
  return "get_group_id(" + runtime_index + ")";
}
string OclDeviceProgram::group_size(uint_tp fixed_index) {
  return "get_num_groups("+ std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::group_size(string runtime_index) {
  return "get_num_groups(" + runtime_index + ")";
}
string OclDeviceProgram::global_id(uint_tp fixed_index) {
  return "get_global_id(" + std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::global_id(string runtime_index) {
  return "get_global_id(" + runtime_index + ")";
}
string OclDeviceProgram::global_size(uint_tp fixed_index) {
  return "get_global_size(" + std::to_string(fixed_index) + ")";
}
string OclDeviceProgram::global_size(string runtime_index) {
  return "get_global_size(" + runtime_index + ")";
}

string local_barrier() {
  return "barrier(CLK_LOCAL_MEM_FENCE)";
}
string global_barrier() {
  return "barrier(CLK_GLOBAL_MEM_FENCE)";
}

string OclDeviceProgram::atomics() {
  stringstream ss;

  // 32 bit atomics
  ss << "#if defined(cl_khr_int32_base_atomics)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_khr_int32_base_atomics : enable"
     << std::endl;
  ss << "#define ATOMICS_32_AVAILABLE" << std::endl;
  ss << "#endif" << std::endl;
  ss << "#if defined(cl_khr_global_int32_base_atomics)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable"
     << std::endl;
  ss << "#define ATOMICS_32_AVAILABLE" << std::endl;
  ss << "#endif" << std::endl;

  // 64 bit atomics
  ss << "#if defined(cl_khr_int64_base_atomics)" << std::endl;
  ss << "#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable"
     << std::endl;
  ss << "#define ATOMICS_64_AVAILABLE" << std::endl;
  ss << "#endif" << std::endl;

  vector<string> atomic_datatypes({"half", "float", "double"});
  vector<string> atomic_funcs({ "add", "sub", "mul", "div" });
  vector<string> atomic_ops({ "+", "-", "*", "/" });

  // OpenCL atomics, derived from:
  // https://streamcomputing.eu/blog/2016-02-09/
  // atomic-operations-for-floats-in-opencl-improved/
  for (int j = 0; j < atomic_datatypes.size(); ++j) {
    string atomic_datatype = atomic_datatypes[j];
    if (atomic_datatype == "float" || atomic_datatype == "half") {
      ss << "#if defined(ATOMICS_32_AVAILABLE)" << std::endl;
    } else if (atomic_datatype == "double") {
      ss << "#if defined(ATOMICS_64_AVAILABLE)" << std::endl;
    }
    for (int i = 0; i < atomic_funcs.size(); ++i) {
      ss << "inline void caffe_gpu_atomic_"
         << atomic_datatype << "_" << atomic_funcs[i];
      ss << "(volatile __global " << atomic_datatype << "* source, const "
         << atomic_datatype << " operand) {"
         << std::endl;
      ss << "union {" << std::endl;
      if (atomic_datatype == "double") {
        ss << "unsigned long intVal;" << std::endl;
      } else {
        ss << "unsigned int intVal;" << std::endl;
      }
      if (atomic_datatype == "half") {
        ss << atomic_datatype << " floatVal[2];" << std::endl;
      } else {
        ss << atomic_datatype << " floatVal[1];" << std::endl;
      }
      ss << "} next, expected, current;" << std::endl;
      ss << "current.floatVal[0] = *source;" << std::endl;
      if (atomic_datatype == "half") {
        ss << "current.floatVal[1] = *(source + 1);" << std::endl;
      }
      ss << "do {" << std::endl;
      ss << "expected.intVal = current.intVal;" << std::endl;
      ss << "next.floatVal[0] = expected.floatVal[0] "
         << atomic_ops[i] << " operand;" << std::endl;
      if (atomic_datatype == "half") {
        ss << "next.floatVal[1] = expected.floatVal[1]; " << std::endl;
      }
      ss << "current.intVal = ";
      if (atomic_datatype == "double") {
        ss << "atom_cmpxchg((volatile __global unsigned long *)";
      } else {
        ss << "atomic_cmpxchg((volatile __global unsigned int *)";
      }
      ss << "source, expected.intVal, next.intVal);" << std::endl;
      ss << "} while (current.intVal != expected.intVal);" << std::endl;
      ss << "}" << std::endl;
    }
    ss << "#endif" << std::endl;
  }
  return ss.str();
}

inline string pointer_suffix(uint64_t flags) {
  if ((flags & KERNEL_ARG_GLOBAL_MEM) == KERNEL_ARG_GLOBAL_MEM ||
      (flags & KERNEL_ARG_LOCAL_MEM) == KERNEL_ARG_LOCAL_MEM) {
    return "*";
  }
  return "";
}

string OclDeviceProgram::kernel_arg_type_void(uint64_t flags) {
  return "void" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_bool(uint64_t flags) {
  if ((flags & KERNEL_ARG_GLOBAL_MEM) == KERNEL_ARG_GLOBAL_MEM ||
      (flags & KERNEL_ARG_LOCAL_MEM) == KERNEL_ARG_LOCAL_MEM) {
    return "bool" + pointer_suffix(flags);
  } else {
    // OpenCL kernel arguments are not supporting booleans, convert to int8_t
    return "int8_t";
  }
}
string OclDeviceProgram::kernel_arg_type_char(uint64_t flags) {
  return "char" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_half(uint64_t flags) {
  if ((flags & KERNEL_ARG_GLOBAL_MEM) == KERNEL_ARG_GLOBAL_MEM ||
      (flags & KERNEL_ARG_LOCAL_MEM) == KERNEL_ARG_LOCAL_MEM) {
    return "half" + pointer_suffix(flags);
  } else {
    // OpenCL kernel arguments are not supporting halfs, convert to float
    return "float";
  }
}
string OclDeviceProgram::kernel_arg_type_float(uint64_t flags) {
  return "float" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_double(uint64_t flags) {
  return "double" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_int8_t(uint64_t flags) {
  return "int8_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_int16_t(uint64_t flags) {
  return "int16_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_int32_t(uint64_t flags) {
  return "int32_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_int64_t(uint64_t flags) {
  return "int64_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_uint8_t(uint64_t flags) {
  return "uint8_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_uint16_t(uint64_t flags) {
  return "uint16_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_uint32_t(uint64_t flags) {
  return "uint32_t" + pointer_suffix(flags);
}
string OclDeviceProgram::kernel_arg_type_uint64_t(uint64_t flags) {
  return "uint64_t" + pointer_suffix(flags);
}

#endif  // USE_OPENCL

}
