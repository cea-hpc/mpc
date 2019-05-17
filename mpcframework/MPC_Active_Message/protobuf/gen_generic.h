#ifndef GEN_GENERIC_H
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/compiler/plugin.h>
#define GEN_GENERIC_H 

namespace arpc {

class Options
{
	public:
	std::string ns;
	std::string ns_prefix;
};

class ArpcGenerator : public google::protobuf::compiler::CodeGenerator
{
	public:

		ArpcGenerator() : google::protobuf::compiler::CodeGenerator() { }
		virtual ~ArpcGenerator() {}

		virtual bool Generate(
				const google::protobuf::FileDescriptor * file,
				const std::string & parameter,
				google::protobuf::compiler::GeneratorContext * generator_context,
				std::string * error) const;
};

static inline std::string upper_str(std::string str)
{
	for(size_t i = 0; i < str.length(); i++)
	{
		str[i] = ::toupper(str[i]);
	}
	return str;
}

static inline std::string strip_ext(const std::string& str, const std::string& ext)
{
	size_t ext_start = str.length() - ext.length();
	if(ext_start > 0 && !str.compare(ext_start, std:: string::npos, ext))
	{
		return str.substr(0, ext_start);
	}
	return str;
}

static inline std::string strip_path(const std::string& str)
{
	size_t path_end = str.find_last_of("/");
	if(path_end > 0 && path_end < str.length())
	{
		return str.substr(path_end);
	}
	return str;
}

/* copied from grpc/src/compiler/generator_helpers.h */
static inline std::vector<std::string> tokenize(const std:: string& input, const std:: string& delimiters) {
	std::vector<std::string> tokens;
	size_t pos, last_pos = 0;

	for (;;) {
		bool done = false;
		pos = input.find_first_of(delimiters, last_pos);
		if (pos == std:: string::npos) {
			done = true;
			pos = input.length();
		}

		tokens.push_back(input.substr(last_pos, pos - last_pos));
		if (done) return tokens;

		last_pos = pos + 1;
	}
}

static inline std::string str_replace(const std:: string& input, const std:: string& pattern, const std:: string& replace)
{
	size_t pos = 0, last = 0;
	std::string res = input;
	for(;;)
	{
		pos = res.find(pattern, last);
		if(pos == std:: string::npos)
		{
			return res;
		}
		res.replace(pos, 1, replace);

		last = pos+replace.length();
	}

}

}

#endif /* ifndef GEN_GENERIC_H */
