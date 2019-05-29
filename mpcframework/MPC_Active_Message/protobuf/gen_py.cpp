#include <locale>
#include "gen_py.h"

using namespace std;
using namespace google::protobuf;

namespace arpc {

ArpcPython::ArpcPython(const google::protobuf::FileDescriptor *pf)
{
	filename = strip_path(strip_ext(pf->name(), ".proto"));
}

std::ostringstream ArpcPython::unrollMethod(const ServiceDescriptor * serv, bool should_raise = false)
{
	std::ostringstream method_conversion;

	for(auto i = 0; i < serv->method_count(); i++)
	{
		const MethodDescriptor * method = serv->method(i);
		std::string name, in, out;
		name = method->name();
		in = opts.ns_prefix + method->input_type()->name();
		out = opts.ns_prefix + method->output_type()->name();

		stream << "\tdef " << name << "(self, context, request):"<< std::endl;
		if(should_raise)
		{
			stream << "\t\traise NotImplementedError(\""<< serv->name() << "." << name << "() function not implemented !\")" << std::endl << std::endl;
		}
		else
		{
		stream
			<< "\t\tif(not isinstance(request, "<< in <<")):" << std::endl
			<< "\t\t\traise TypeError(\"'request' is not an instance of "<< in << "\")" << std::endl
			;

		if(in.compare("Empty") != 0)
		{
			stream
				<< "\t\traw_req = request.SerializeToString()"<< std::endl
				<< "\t\treq_sz = len(raw_req)" << std::endl
				;
		}

		ostringstream resp_output;
		if(out.compare("Empty") != 0)
		{
			resp_output
				<< "\t\tresponse = " << out << "()" << std::endl
				<< "\t\tif(len(raw_resp) > 0):" << std::endl
				<< "\t\t\tresponse.ParseFromString(raw_resp)" << std::endl
				<< "\t\telse:" << std::endl
				<< "\t\t\tresponse.Clear()" << std::endl
				;
		}

		stream
			<< "\t\traw_resp = arpc4py.emit_call(" << serv->index() << ", " << method->index() << ", context.dest, raw_req, " << (out.compare("Empty") == 0 ? "0" : "1") << ")" << std::endl
			;

		stream
			<< resp_output.str()
			<< "\t\treturn response" <<std::endl
			;
		}

		method_conversion
			<< "\t\tif rpc == " << method->index() << ": req = " << in << "()" << std::endl
			;
		continue;

		if(in.compare("Empty") != 0)
		{
			method_conversion
				<< "\t\t\t\t\t\treq = new "<<opts.ns_prefix<<in << ";"               << std::endl
				<< "\t\t\t\t\t\tif(req_size > 0)"                        << std::endl
				<< "\t\t\t\t\t\t\treq->ParseFromString((char*)request);" << std::endl
				<< "\t\t\t\t\t\telse"                                    << std::endl
				<< "\t\t\t\t\t\t\treq->Clear();"                         << std::endl
				;
		}

		if(out.compare("Empty") != 0)
		{
			method_conversion
				<< "\t\t\t\t\t\tresp = new "<<opts.ns_prefix<<out << ";" << std::endl
				<< "\t\t\t\t\t\tstd::string out;"            << std::endl
				;

		}
		else
		{
			method_conversion
				<< "\t\t\t\t\t\t*resp_size = 0;"        << std::endl
				<< "\t\t\t\t\t\t*response_addr = NULL;" << std::endl
				;
		}

		method_conversion << "\t\t\t\t\t\tret = r->" << name << "(ctx, req, resp);" << std::endl;

		if(out.compare("Empty") != 0)
		{
			method_conversion
				<< "\t\t\t\t\t\tresp->SerializeToString(&out);"        << std::endl
				<< "\t\t\t\t\t\t*resp_size = out.size();"              << std::endl
				<< "\t\t\t\t\t\t*response_addr = strdup(out.c_str());" << std::endl
				;
		}

		method_conversion
			<< "\t\t\t\t\t\tbreak;" << std::endl
			<< "\t\t\t\t\t}"        << std::endl
			;
	}
	return method_conversion;
}


std::ostringstream ArpcPython::unrollService(const FileDescriptor * pfile)
{
	std::ostringstream service_conversion;

	for(auto i = 0; i < pfile->service_count(); i++)
	{
		auto serv = pfile->service(i);
		stream 
			<< "class " << serv->name() << "Recv:" << std::endl
			;

		service_conversion 
			<< "\tif serv == " << serv->index() << ":" << std::endl
			<< unrollMethod(serv, true).str();

		stream
			<< "\tdef __init__(self):" << std::endl
			<< "\t\tif(" << serv->index() << " not in servDict):" << std::endl
			<< "\t\t\tservDict["<< serv->index()<<"] = dict()" << std::endl
			;

		for(auto j = 0; j < serv->method_count(); j++)
		{
		stream
			<< "\t\t\tservDict[" << serv->index() << "]["<< serv->method(j)->index() << "] = self." << serv->method(j)->name()   << std::endl
			;
		}

		stream
			<< std::endl
			<< "\tdef wait(self, max):" << std::endl
			<< "\t\tcnt = 0" << std::endl
			<< "\t\twhile(cnt < max or max == -1 ):" << std::endl
			<< "\t\t\tarpc4py.polling_request()" << std::endl
			<< "\t\t\tcnt+=1" << std::endl
			<< std::endl
			<< "class " << serv->name() << "Send:" << std::endl
			<< "\tdef __init__(self):" << std::endl
			<< "\t\tprint(\"Create client for Service " << serv->name() << "\")" << std::endl
			<< std::endl
			;

		unrollMethod(serv, false);
	}
	return  service_conversion;
}

void ArpcPython::buildScript(const FileDescriptor * pfile)
{
	std::ostringstream service_conversion;
	stream
		<< "import sys" << std::endl
		<< "import ctypes" << std::endl
		<< "sys.setdlopenflags(sys.getdlopenflags() | ctypes.RTLD_GLOBAL)" << std::endl
		<< "import arpc4py" << std::endl
		<< "import " << opts.ns << std::endl
		<< std::endl
		<< "servDict = dict()" << std::endl
		<< std::endl
		<< "class Context:" << std::endl
		<< "\tdef __init__(self, dest=-1):"<<std::endl
		<< "\t\tself.dest = dest" << std::endl
		<< std::endl
		;


	for(auto i = 0; i < pfile->service_count(); i++)
	{
		auto serv = pfile->service(i);
		service_conversion = unrollService(pfile);
	}

	stream
		<< std::endl
		<< "def __convert_c_to_python(serv, rpc, dest, raw_req):" << std::endl
		<< "\tif serv not in servDict:" << std::endl
		<< "\t\traise KeyError(\"Service not found !\")" << std::endl
		<< "\tif rpc not in servDict[serv]:" << std::endl
		<< "\t\traise KeyError(\"Method not found for this Service !\")" << std::endl
		<< service_conversion.str() << std::endl
		<< "\treturn servDict[serv][rpc](Context(dest), req.ParseFromString(raw_req)).SerializeToString()" << std::endl

		<< std::endl
		<< "arpc4py.register_callback(callback=__convert_c_to_python)" << std::endl
		;
}
}
