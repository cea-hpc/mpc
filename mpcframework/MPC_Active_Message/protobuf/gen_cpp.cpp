#include <locale>
#include "gen_cpp.h"

using namespace std;
using namespace google::protobuf;

namespace arpc {

ArpcCpp::ArpcCpp(const google::protobuf::FileDescriptor *pf)
{
	filename = strip_path(strip_ext(pf->name(), ".proto"));
}

std::ostringstream ArpcCpp::unrollSourceMethod(const ServiceDescriptor * serv, bool pure_virtual = false)
{
	std::ostringstream method_conversion;

	for(auto i = 0; i < serv->method_count(); i++)
	{
		const MethodDescriptor * method = serv->method(i);
		std::string name, in, out;
		name = method->name();
		in = method->input_type()->name();
		out = method->output_type()->name();

		src_stream
			<< "int " << serv->name()<< "::Send::" << name << "( arpc::Context* ctx, " << in <<"* request, " << out << "* response) {"<< std::endl
			<< "\tconst char* raw_req = NULL;"               << std::endl
			<< "\tvoid* raw_resp = NULL;"                    << std::endl
			<< "\tsize_t req_size = 0, resp_size = 0;"       << std::endl
			<< "\tint ret;"                                  << std::endl
			<< "\tctx->rpcode = " << method->index() << ";"  << std::endl
			<< "\tctx->srvcode = " << serv->name() << "_T; " << std::endl
			;

		if(in.compare("Empty") != 0)
		{
			src_stream
				<< "\treq_size = request->SpaceUsed();"<< std::endl
				<< "\tstd::string tmp_str;" << std::endl
				<< "\trequest->SerializeToString(&tmp_str);"<< std::endl
				<< "\traw_req = tmp_str.c_str();" << std::endl
				<< "\treq_size = tmp_str.size();" << std::endl
				;
		}

		ostringstream resp_output;
		if(out.compare("Empty") != 0)
		{
			src_stream
				<< "\tresp_size = sizeof(" << out << ");" << std::endl
				;

			resp_output
				<< "\tif(resp_size > 0)" << std::endl
				<< "\t\tresponse->ParseFromString((char*)raw_resp);" << std::endl
				<< "\telse" << std::endl
				<< "\t\tresponse->Clear();" << std::endl
				;
		}

		src_stream
			<< "\tret = arpc_emit_call(ctx, raw_req, req_size, " << (!out.compare("Empty") ? "NULL" : "&raw_resp") << ", &resp_size);" << std::endl
			<< "\tif(ret) return ret; /* error */" << std::endl
			;

		src_stream
			<< resp_output.str()
			<< "\treturn 0;" <<std::endl
			<<"}" << std::endl
			;

		method_conversion
			<< "\t\t\t\t\tcase " << method->index() << ": {"                      << std::endl
			<< "\t\t\t\t\t\t" << opts.ns_prefix<<in << "* req = NULL;"            << std::endl
			<< "\t\t\t\t\t\t" << opts.ns_prefix<<out << "* resp = NULL;"          << std::endl
			;

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


void ArpcCpp::unrollHeaderMethod(const ServiceDescriptor * serv, bool pure_virtual = false)
{
	for(auto i = 0; i < serv->method_count(); i++)
	{
		const MethodDescriptor * method = serv->method(i);
		std::string name, in, out;
		name = method->name();
		in = method->input_type()->name();
		out = method->output_type()->name();

		hdr_stream << "\tvirtual int " << name << "( arpc::Context* ctx, " << in <<"* request, " << out << "* response)" << ((pure_virtual) ? " = 0":"") << ";"<< std::endl;
	}
}

std::ostringstream ArpcCpp::unrollSourceService(const FileDescriptor * pfile)
{
	std::ostringstream service_conversion;
	
	for(auto i = 0; i < pfile->service_count(); i++)
	{
		auto serv = pfile->service(i);
		service_conversion
			<< "\t\t\tcase " << opts.ns_prefix<< serv->name() << "_T: {" << std::endl
			<< "\t\t\t\t"<<opts.ns_prefix<<serv->name()<<"::Recv* r = ("<<opts.ns_prefix<<serv->name()<<"::Recv*)srv;" << std::endl
			<< "\t\t\t\tswitch(rpcode) {" << std::endl
			<< unrollSourceMethod(serv).str()
			<< "\t\t\t\t\tdefault:" << std::endl
			<< "\t\t\t\t\t\tstd::cerr << \"Unknown RPC '\" << rpcode << \"' for service "<< serv->name()  <<"\" << std::endl;" << std::endl
			<< "\t\t\t\t\t\tstd::abort();" << std::endl
			<< "\t\t\t\t\t\tbreak;" << std::endl
			<< "\t\t\t\t}" << std::endl
			<< "\t\t\t\tbreak;" << std::endl
			<< "\t\t\t}" << std::endl
	;
	}
	return service_conversion;
}

void ArpcCpp::unrollHeaderService(const FileDescriptor * pfile)
{
	for(auto i = 0; i < pfile->service_count(); i++)
	{
		auto serv = pfile->service(i);
		std::string serv_name = serv->name();
		hdr_stream
			<< "namespace "<< serv_name << " {"              << std::endl
			<< "class Send : public Service {"         << std::endl
			<< "public:"                                     << std::endl
			<< "\tSend() : Service(" << serv_name << "_T) {}"  << std::endl
			<< "\tvirtual ~Send() {}"                        << std::endl
			;
		
		unrollHeaderMethod(serv, false);

		hdr_stream
			<< "}; /* class Send for " << serv_name << " */" << std::endl
			<< "class Recv : public Service {"         << std::endl
			<< "public:"                                     << std::endl
			<< "\tRecv() : Service(" << serv_name << "_T) {}"  << std::endl
			<< "\tvirtual ~Recv() {}"                        << std::endl
			;

		unrollHeaderMethod(serv, true);

		hdr_stream
			<< "}; /* class Send for " << serv_name << " */" << std::endl
			<< "}"                                           << std::endl
			;
	}

}

void ArpcCpp::buildHeader(const FileDescriptor * pfile)
{
	string filetag = upper_str(filename);
	// 1. add the guard
	// 3. include protobuf header file
	// 2. Add generic include
	hdr_stream 
		<< "#ifndef "         << filetag   << "_ARPC_H_" << std::endl
		<< "#define "         << filetag   << "_ARPC_H_" << std::endl
		<< "#include \""      << filename  << ".pb.h\""  << std::endl
		<< "#include<list>" << std::endl
		<< "extern \"C\" {" << std::endl
		<< "#include <arpc_common.h>" << std::endl
		<< "#include <arpc.h>" << std::endl
		<< "}" << std::endl
		<< "namespace arpc {" << std::endl
		<< "typedef ::sctk_arpc_context Context;"          << std::endl
		<< "} /* namespace arpc */" << std::endl 
		;

	// TODO: Add srv_types

	if(!opts.ns.empty())
		hdr_stream << "namespace " << opts.ns << " {"<< std::endl;
	hdr_stream << "enum srv_type_e {" << std::endl
	;

	for(auto i = 0; i < pfile->service_count(); i++)
		hdr_stream << "\t" << pfile->service(i)->name() << "_T ," << std::endl;

	hdr_stream
		<< ""
		<< "__ARPC_SRV_TYPES_NB" << std::endl
		<< "};" << std::endl
		<< "class Service {"                                     << std::endl
		<< "protected:"                                          << std::endl
		<< "\tenum srv_type_e _type;"                            << std::endl
		<< "public:"                                             << std::endl
		<< "\tService(srv_type_e t) : _type(t) {arpc_init(); }"  << std::endl
		<< "\tenum srv_type_e __type() { return _type; }"        << std::endl
		<< "}; /* class Service */"                              << std::endl << std::endl
		<< "class ServicePool {"                                 << std::endl
		<< "private:"                                            << std::endl
		<< "\tstd::map<srv_type_e, Service*> srv_array;"         << std::endl
		<< "public:"                                             << std::endl
		<< "\tvoid add(Service* srv) {"                          << std::endl
		<< "\t        srv_array.insert("                         << std::endl
		<< "\t            std::pair<srv_type_e,"                 << std::endl
		<< "\t            Service*>(srv->__type(), srv));" << std::endl
		<< "\t}"                                                 << std::endl
		<< "\tService* find(enum srv_type_e k) {"                << std::endl
		<< "\t        return srv_array[k];"                      << std::endl
		<< "\t}"                                                 << std::endl
		<< "}; /* class ServicePool */"                          << std::endl
		;

	/* TODO: for each service */
	ArpcCpp::unrollHeaderService(pfile);
	if(!opts.ns.empty())
		hdr_stream << "} /* namespace " << opts.ns << " */" << std::endl;

	hdr_stream 
		<< "int arpc_c_to_cxx_converter("                << std::endl
		<< "        sctk_arpc_context_t*, const void *," << std::endl
		<< "        size_t, void **, "                   << std::endl
		<< "        size_t*);"                           << std::endl
		<< "#endif /* " << filetag << "_ARPC_H_ */"      << std::endl
		;


}

void ArpcCpp::buildSource(const FileDescriptor * pfile)
{
	std::ostringstream serv_conversion;
	src_stream 
		<< "#include \""      << filename  << ".pb.h\""        << std::endl
		<< "#include \""      << filename  << ".arpc.pb.h\"\n" << std::endl
		;
	
	if(!opts.ns.empty())
	{
		src_stream << "namespace " << opts.ns << " {"<< std::endl;
	}
	
	serv_conversion << ArpcCpp::unrollSourceService(pfile).str();
	
	if(!opts.ns.empty())
		src_stream << "} /* namespace " << opts.ns << " */"<< std::endl;

	// 1. include protobuf header file
	// 2. include arpc header file
	// 4. add own headers
	// 5. add namespace && class impl.
	//
	src_stream
		<< "int arpc_c_to_cxx_converter("                                         << std::endl
		<< "        sctk_arpc_context_t* ctx, const void * request,"              << std::endl
		<< "        size_t req_size, void ** response_addr, "                     << std::endl
		<< "        size_t*resp_size){"                                           << std::endl
		<< "\t"<<opts.ns_prefix<<"ServicePool * pool = ("<<opts.ns_prefix<<"ServicePool*)ctx->cxx_pool;"  << std::endl
		<< "\t"<<opts.ns_prefix<<"srv_type_e srvcode = ("<<opts.ns_prefix<<"srv_type_e)ctx->srvcode;"     << std::endl
		<< "\t"<<opts.ns_prefix<<"Service* srv;"                                              << std::endl
		<< "\tint ret = -1, rpcode = ctx->rpcode;"                  << std::endl  << std::endl
		<< "\tif(!pool) return 1;"                                                << std::endl
		<< "\tsrv = pool->find(srvcode);"                                         << std::endl
		<< "\tswitch(srvcode) {"                                                  << std::endl
		<< serv_conversion.str()                                                  << std::endl
		<< "\t\tdefault:"                                                         << std::endl
		<< "\t\t\tstd::cerr << \"Unknown Service Code \" << srvcode <<std::endl;" << std::endl
		<< "\t\t\tstd::abort();"                                                  << std::endl
		<< "\t\t\tbreak;"                                                         << std::endl
		<< "\t}"                                                                  << std::endl
		<< "\treturn ret;"                                                        << std::endl
		<< "}"                                                                    << std::endl
	;
	
}
}
