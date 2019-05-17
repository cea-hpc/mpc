#include "gen_generic.h"
#include "gen_cpp.h"
#include "gen_py.h"

using namespace google::protobuf;
using namespace std;

namespace arpc {

bool ArpcGenerator::Generate(
		const FileDescriptor * file,
		const string & parameter,
		compiler::GeneratorContext * generator_context,
		string * error) const
{

#ifdef ARPC_FOR_PYTHON
	ArpcPython f(file);
#elif defined(ARPC_FOR_CPP)
	ArpcCpp f(file);
#else
#error "Please define ARPC_FOR_PYTHON or ARPC_FOR_CPP when building against this file"
#endif

	if(!file->package().empty())
	{
#ifdef ARPC_FOR_PYTHON
		f.opts.ns = file->package() + "_pb2";
		f.opts.ns_prefix= f.opts.ns + ".";
#elif defined(ARPC_FOR_CPP)
		f.opts.ns = str_replace(file->package(), ".", "::");
		f.opts.ns_prefix = f.opts.ns + "::";
#endif
	}

	/* chunk of code copied from grpc generator */
	if (!parameter.empty()) {
		std::vector<string> parameters_list = tokenize(parameter, ",");
		for (auto parameter_string = parameters_list.begin(); parameter_string != parameters_list.end(); parameter_string++) {
			std::cerr << *parameter_string << std::endl;
			std::vector<string> param = tokenize(*parameter_string, "=");
			/* parse with param[0] & param[1]  : */
			if(param[0] == "something")
			{
				/* TBW from param[1] */
			}
		}
	}

	std::string s;
#ifdef ARPC_FOR_PYTHON
	f.buildScript(file);
	std::string ofile = strip_ext(file->name(), ".proto");
	{
		s = f.getContent();
		std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> fp(generator_context->Open(ofile + "_pb2_arpc.py"));
		google::protobuf::io::CodedOutputStream fpc(fp.get());
		fpc.WriteRaw(s.c_str(), s.size());
	}


#elif defined(ARPC_FOR_CPP)
	f.buildHeader(file);
	f.buildSource(file);

	std::string ofile = strip_ext(file->name(), ".proto");
	{
		s = f.getSourceContent();
		std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> fs(generator_context->Open(ofile + ".arpc.pb.cc"));
		google::protobuf::io::CodedOutputStream fsc(fs.get());
		fsc.WriteRaw(s.c_str(), s.size());

		s = f.getHeaderContent();
		std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> fh(generator_context->Open(ofile + ".arpc.pb.h"));
		google::protobuf::io::CodedOutputStream fhc(fh.get());
		fhc.WriteRaw(s.c_str(), s.size());
	}
#endif
	return true;
}
}

int main(int argc, char *argv[])
{
	::arpc::ArpcGenerator gen;
	return ::google::protobuf::compiler::PluginMain(argc, argv, &gen);
}
