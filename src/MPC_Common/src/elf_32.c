/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_common_debug.h>
#include <mpc_common_debugger.h>
#include <debugger.h>

#include <string.h>
#include <stdlib.h>
#include <sctk_alloc.h>
#include <assert.h>

#define DW_FORM_
#define DW_FORM_addr 0x01
#define DW_FORM_data2 0x05
#define DW_FORM_data4 0x06
#define DW_FORM_data8 0x07
#define DW_FORM_string 0x08
#define DW_FORM_data1 0x0b
#define DW_FORM_strp 0x0e

#if defined( DEBUG )
static char *get_arch( int machine )
{
	switch ( machine )
	{
#ifdef EM_NONE
		case EM_NONE:
			return "No machine";
#endif
#ifdef EM_M32
		case EM_M32:
			return "AT&T WE 32100";
#endif
#ifdef EM_SPARC
		case EM_SPARC:
			return "SUN SPARC";
#endif
#ifdef EM_386
		case EM_386:
			return "Intel 80386";
#endif
#ifdef EM_68K
		case EM_68K:
			return "Motorola m68k family";
#endif
#ifdef EM_88K
		case EM_88K:
			return "Motorola m88k family";
#endif
#ifdef EM_860
		case EM_860:
			return "Intel 80860";
#endif
#ifdef EM_MIPS
		case EM_MIPS:
			return "MIPS R3000 big-endian";
#endif
#ifdef EM_S370
		case EM_S370:
			return "IBM System/370";
#endif
#ifdef EM_MIPS_RS3_LE
		case EM_MIPS_RS3_LE:
			return "MIPS R3000 little-endian";
#endif
#ifdef EM_PARISC
		case EM_PARISC:
			return "HPPA";
#endif
#ifdef EM_VPP500
		case EM_VPP500:
			return "Fujitsu VPP500";
#endif
#ifdef EM_SPARC32PLUS
		case EM_SPARC32PLUS:
			return "Sun's \"v8plus\"";
#endif
#ifdef EM_960
		case EM_960:
			return "Intel 80960";
#endif
#ifdef EM_PPC
		case EM_PPC:
			return "PowerPC";
#endif
#ifdef EM_PPC64
		case EM_PPC64:
			return "PowerPC 64-bit";
#endif
#ifdef EM_S390
		case EM_S390:
			return "IBM S390";
#endif
#ifdef EM_V800
		case EM_V800:
			return "NEC V800 series";
#endif
#ifdef EM_FR20
		case EM_FR20:
			return "Fujitsu FR20";
#endif
#ifdef EM_RH32
		case EM_RH32:
			return "TRW RH-32";
#endif
#ifdef EM_RCE
		case EM_RCE:
			return "Motorola RCE";
#endif
#ifdef EM_ARM
		case EM_ARM:
			return "ARM";
#endif
#ifdef EM_FAKE_ALPHA
		case EM_FAKE_ALPHA:
			return "Digital Alpha";
#endif
#ifdef EM_SH
		case EM_SH:
			return "Hitachi SH";
#endif
#ifdef EM_SPARCV9
		case EM_SPARCV9:
			return "SPARC v9 64-bit";
#endif
#ifdef EM_TRICORE
		case EM_TRICORE:
			return "Siemens Tricore";
#endif
#ifdef EM_ARC
		case EM_ARC:
			return "Argonaut RISC Core";
#endif
#ifdef EM_H8_300
		case EM_H8_300:
			return "Hitachi H8/300";
#endif
#ifdef EM_H8_300H
		case EM_H8_300H:
			return "Hitachi H8/300H";
#endif
#ifdef EM_H8S
		case EM_H8S:
			return "Hitachi H8S";
#endif
#ifdef EM_H8_500
		case EM_H8_500:
			return "Hitachi H8/500";
#endif
#ifdef EM_IA_64
		case EM_IA_64:
			return "Intel IA64";
#endif
#ifdef EM_MIPS_X
		case EM_MIPS_X:
			return "Stanford MIPS-X";
#endif
#ifdef EM_COLDFIRE
		case EM_COLDFIRE:
			return "Motorola Coldfire";
#endif
#ifdef EM_68HC12
		case EM_68HC12:
			return "Motorola M68HC12";
#endif
#ifdef EM_MMA
		case EM_MMA:
			return "Fujitsu MMA Multimedia Accelerator";
#endif
#ifdef EM_PCP
		case EM_PCP:
			return "Siemens PCP";
#endif
#ifdef EM_NCPU
		case EM_NCPU:
			return "Sony nCPU embeeded RISC";
#endif
#ifdef EM_NDR1
		case EM_NDR1:
			return "Denso NDR1 microprocessor";
#endif
#ifdef EM_STARCORE
		case EM_STARCORE:
			return "Motorola Start*Core processor";
#endif
#ifdef EM_ME16
		case EM_ME16:
			return "Toyota ME16 processor";
#endif
#ifdef EM_ST100
		case EM_ST100:
			return "STMicroelectronic ST100 processor";
#endif
#ifdef EM_TINYJ
		case EM_TINYJ:
			return "Advanced Logic Corp. Tinyj emb.fam";
#endif
#ifdef EM_X86_64
		case EM_X86_64:
			return "AMD x86-64 architecture";
#endif
#ifdef EM_PDSP
		case EM_PDSP:
			return "Sony DSP Processor";
#endif
#ifdef EM_FX66
		case EM_FX66:
			return "Siemens FX66 microcontroller";
#endif
#ifdef EM_ST9PLUS
		case EM_ST9PLUS:
			return "STMicroelectronics ST9+ 8/16 mc";
#endif
#ifdef EM_ST7
		case EM_ST7:
			return "STmicroelectronics ST7 8 bit mc";
#endif
#ifdef EM_68HC16
		case EM_68HC16:
			return "Motorola MC68HC16 microcontroller";
#endif
#ifdef EM_68HC11
		case EM_68HC11:
			return "Motorola MC68HC11 microcontroller";
#endif
#ifdef EM_68HC08
		case EM_68HC08:
			return "Motorola MC68HC08 microcontroller";
#endif
#ifdef EM_68HC05
		case EM_68HC05:
			return "Motorola MC68HC05 microcontroller";
#endif
#ifdef EM_SVX
		case EM_SVX:
			return "Silicon Graphics SVx";
#endif
#ifdef EM_ST19
		case EM_ST19:
			return "STMicroelectronics ST19 8 bit mc";
#endif
#ifdef EM_VAX
		case EM_VAX:
			return "Digital VAX";
#endif
#ifdef EM_CRIS
		case EM_CRIS:
			return "Axis Communications 32-bit embedded processor";
#endif
#ifdef EM_JAVELIN
		case EM_JAVELIN:
			return "Infineon Technologies 32-bit embedded processor";
#endif
#ifdef EM_FIREPATH
		case EM_FIREPATH:
			return "Element 14 64-bit DSP Processor";
#endif
#ifdef EM_ZSP
		case EM_ZSP:
			return "LSI Logic 16-bit DSP Processor";
#endif
#ifdef EM_MMIX
		case EM_MMIX:
			return "Donald Knuth's educational 64-bit processor";
#endif
#ifdef EM_HUANY
		case EM_HUANY:
			return "Harvard University machine-independent object files";
#endif
#ifdef EM_PRISM
		case EM_PRISM:
			return "SiTera Prism";
#endif
#ifdef EM_AVR
		case EM_AVR:
			return "Atmel AVR 8-bit microcontroller";
#endif
#ifdef EM_FR30
		case EM_FR30:
			return "Fujitsu FR30";
#endif
#ifdef EM_D10V
		case EM_D10V:
			return "Mitsubishi D10V";
#endif
#ifdef EM_D30V
		case EM_D30V:
			return "Mitsubishi D30V";
#endif
#ifdef EM_V850
		case EM_V850:
			return "NEC v850";
#endif
#ifdef EM_M32R
		case EM_M32R:
			return "Mitsubishi M32R";
#endif
#ifdef EM_MN10300
		case EM_MN10300:
			return "Matsushita MN10300";
#endif
#ifdef EM_MN10200
		case EM_MN10200:
			return "Matsushita MN10200";
#endif
#ifdef EM_PJ
		case EM_PJ:
			return "picoJava";
#endif
#ifdef EM_OPENRISC
		case EM_OPENRISC:
			return "OpenRISC 32-bit embedded processor";
#endif
#ifdef EM_ARC_A5
		case EM_ARC_A5:
			return "ARC Cores Tangent-A5";
#endif
#ifdef EM_XTENSA
		case EM_XTENSA:
			return "Tensilica Xtensa Architecture";
#endif
	}
	return "Unknown";
}
static int nbbits = 32;
#endif

#if defined( DEBUG )
static void
print_header_32( FILE *output, Elf32_Ehdr *header )
{
	char *tmp;
	fprintf( output, "ELF Header:\n" );
	fprintf( output,
			 "  Magic:   %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
			 header->e_ident[0], header->e_ident[1], header->e_ident[2],
			 header->e_ident[3], header->e_ident[4], header->e_ident[5],
			 header->e_ident[6], header->e_ident[7], header->e_ident[8],
			 header->e_ident[9], header->e_ident[10], header->e_ident[11],
			 header->e_ident[12], header->e_ident[13], header->e_ident[14],
			 header->e_ident[15] );
	fprintf( output, "  Class:                             ELF%d\n", nbbits );
	switch ( header->e_ident[EI_DATA] )
	{
		case ELFDATA2LSB:
			tmp = "2's complement, little endian";
			break;
		case ELFDATA2MSB:
			tmp = "2's complement, big endian";
			break;
		default:
			tmp = "Invalid data encoding";
			break;
	}
	fprintf( output, "  Data:                              %s\n", tmp );
	fprintf( output, "  Version:                           %d (current)\n",
			 header->e_ident[EI_VERSION] );

	switch ( header->e_ident[EI_OSABI] )
	{
#ifdef ELFOSABI_SYSV
		case ELFOSABI_SYSV:
			tmp = "UNIX System V ABI";
			break;
#else
#ifdef ELFOSABI_NONE
		case ELFOSABI_NONE:
			tmp = "UNIX System V ABI";
			break;
#endif
#endif
		case ELFOSABI_HPUX:
			tmp = "HP-UX";
			break;
		case ELFOSABI_NETBSD:
			tmp = "NetBSD";
			break;
		case ELFOSABI_LINUX:
			tmp = "Linux";
			break;
		case ELFOSABI_SOLARIS:
			tmp = "Sun Solaris";
			break;
		case ELFOSABI_AIX:
			tmp = "IBM AIX";
			break;
		case ELFOSABI_IRIX:
			tmp = "SGI Irix";
			break;
		case ELFOSABI_FREEBSD:
			tmp = "FreeBSD";
			break;
		case ELFOSABI_TRU64:
			tmp = "Compaq TRU64 UNIX";
			break;
		case ELFOSABI_MODESTO:
			tmp = "Novell Modesto";
			break;
		case ELFOSABI_OPENBSD:
			tmp = "OpenBSD";
			break;
#ifdef ELFOSABI_ARM
		case ELFOSABI_ARM:
			tmp = "ARM";
			break;
#endif
#ifdef ELFOSABI_STANDALONE
		case ELFOSABI_STANDALONE:
			tmp = "Standalone (embedded) application";
			break;
#endif
		default:
			tmp = "";
			break;
	}
	fprintf( output, "  OS/ABI:                            %s\n", tmp );
	fprintf( output, "  ABI Version:                       %d\n",
			 header->e_ident[EI_ABIVERSION] );
	switch ( header->e_type )
	{
		case ET_NONE:
			tmp = "No file type";
			break;
		case ET_REL:
			tmp = "Relocatable file";
			break;
		case ET_EXEC:
			tmp = "Executable file";
			break;
		case ET_DYN:
			tmp = "Shared object file";
			break;
		case ET_CORE:
			tmp = "Core file";
			break;
		case ET_NUM:
			tmp = "Number of defined types";
			break;
		case ET_LOOS:
			tmp = "OS-specific range start";
			break;
		case ET_HIOS:
			tmp = "OS-specific range end";
			break;
		case ET_LOPROC:
			tmp = "Processor-specific range start";
			break;
		case ET_HIPROC:
			tmp = "Processor-specific range end";
			break;
		default:
			tmp = "";
			break;
	}
	fprintf( output, "  Type:                              %s\n", tmp );
	fprintf( output, "  Machine:                           %s\n",
			 get_arch( header->e_machine ) );
	fprintf( output, "  Version:                           %p\n",
			 (void *) header->e_version );
	fprintf( output, "  Entry point address:               %p\n",
			 (void *) header->e_entry );
	fprintf( output,
			 "  Start of program headers:          %d (bytes into file)\n",
			 header->e_phoff );
	fprintf( output,
			 "  Start of section headers:          %d (bytes into file)\n",
			 header->e_shoff );
	fprintf( output, "  Flags:                             %p\n",
			 (void *) header->e_flags );
	fprintf( output, "  Size of this header:               %d (bytes)\n",
			 header->e_ehsize );
	fprintf( output, "  Size of program headers:           %d (bytes)\n",
			 header->e_phentsize );
	fprintf( output, "  Number of program headers:         %d\n",
			 header->e_phnum );
	fprintf( output, "  Size of section headers:           %d (bytes)\n",
			 header->e_shentsize );
	fprintf( output, "  Number of section headers:         %d\n",
			 header->e_shnum );
	fprintf( output, "  Section header string table index: %d\n",
			 header->e_shstrndx );

	/*   fprintf(stderr,"architecture %s %d bits\n",get_arch(header->e_machine),nbbits); */
}
#endif

void sctk_read_elf_header_32( elf_class_t *c )
{

	int ret = fread( &( c->header.h32 ), sizeof( Elf32_Ehdr ), 1, c->fd );

	if( ret == 1)
	{
		if ( c->header.h32.e_type != ET_EXEC )
		{
			c->is_lib = 1;
		}
	}
	/*   print_header_32(stderr,&(c->header.h32)); */
}

#if defined( DEBUG )
static void
print_section_32( FILE *output, elf_class_t *c )
{
	int i;
	/*
     Section Headers:
     [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
     [ 0]                   NULL            00000000 000000 000000 00      0   0  0
   */
	fprintf( output,
			 "There are %d section headers, starting at offset %p:\n\n",
			 c->header.h32.e_shnum, (void *) c->header.h32.e_shoff );

	fprintf( output, "Section Headers:\n" );
	fprintf( output,
			 "  [Nr] Name                      Type            Addr     Off    Size   ES Flg Lk Inf Al:\n" );
	for ( i = 0; i < c->header.h32.e_shnum; i++ )
	{
		char *t = "";
		switch ( c->sections[i].h32.sh_type )
		{
			case SHT_NULL:
				t = "NULL";
				break;
			case SHT_PROGBITS:
				t = "PROGBITS";
				break;
			case SHT_SYMTAB:
				t = "SYMTAB";
				break;
			case SHT_STRTAB:
				t = "STRTAB";
				break;
			case SHT_RELA:
				t = "RELA";
				break;
			case SHT_HASH:
				t = "HASH";
				break;
			case SHT_DYNAMIC:
				t = "DYNAMIC";
				break;
			case SHT_NOTE:
				t = "NOTE";
				break;
			case SHT_NOBITS:
				t = "NOBITS";
				break;
			case SHT_REL:
				t = "REL";
				break;
			case SHT_SHLIB:
				t = "SHLIB";
				break;
			case SHT_DYNSYM:
				t = "DYNSYM";
				break;
			case SHT_INIT_ARRAY:
				t = "INIT_ARRAY";
				break;
			case SHT_FINI_ARRAY:
				t = "FINI_ARRAY";
				break;
			case SHT_PREINIT_ARRAY:
				t = "PREINIT_ARRAY";
				break;
			case SHT_GROUP:
				t = "GROUP";
				break;
			case SHT_SYMTAB_SHNDX:
				t = "SYMTAB_SHNDX";
				break;
			case SHT_NUM:
				t = "NUM";
				break;
			case SHT_LOOS:
				t = "LOOS";
				break;
			case SHT_GNU_LIBLIST:
				t = "GNU_LIBLIST";
				break;
			case SHT_CHECKSUM:
				t = "CHECKSUM";
				break;
			case SHT_LOSUNW:
				t = "LOSUNW";
				break;
			case SHT_SUNW_COMDAT:
				t = "SUNW_COMDAT";
				break;
			case SHT_SUNW_syminfo:
				t = "SUNW_syminfo";
				break;
			case SHT_GNU_verdef:
				t = "GNU_verdef";
				break;
			case SHT_GNU_verneed:
				t = "GNU_verneed";
				break;
			case SHT_HIOS:
				t = "HIOS";
				break;
			case SHT_LOPROC:
				t = "LOPROC";
				break;
			case SHT_HIPROC:
				t = "HIPROC";
				break;
			case SHT_LOUSER:
				t = "LOUSER";
				break;
			case SHT_HIUSER:
				t = "HIUSER";
				break;
		}
		fprintf( output, "  [%2d] %-25s %-15s %08x %06x %06x\n",
				 (int) i, &( c->string_table[c->sections[i].h32.sh_name] ),
				 t,
				 (unsigned int) ( c->sections[i].h32.sh_addr ),
				 (unsigned int) ( c->sections[i].h32.sh_offset ),
				 (unsigned int) ( c->sections[i].h32.sh_size ) );
	}
}
#endif

static long
get_bytes( char *ptr, int nb )
{
	long word = 0;

	memcpy( &word, ptr, nb );

	return (long) word;
}

static inline unsigned long int
read_leb128( unsigned char *data, unsigned int *length_return, int sign )
{
	unsigned long int result = 0;
	unsigned int num_read = 0;
	unsigned int shift = 0;
	unsigned char byte;

	do
	{
		byte = *data++;
		num_read++;
		result |= ( (unsigned long int) ( byte & 0x7f ) ) << shift;

		shift += 7;
	} while ( byte & 0x80 );

	if ( length_return != NULL )
	{
		*length_return = num_read;
	}

	if ( sign && ( shift < 8 * sizeof( result ) ) && ( byte & 0x40 ) )
	{
		unsigned long ones_bitmask = ~0UL;
		result |= ones_bitmask << shift;
	}

	return result;
}
static char *
fecth_indirect_string( elf_class_t *c, unsigned long offset )
{
	char *tmp;
	if ( c->debug_str == NULL )
	{
		return "Unable to find string";
	}
	tmp = ( (char *) ( c->debug_str ) ) + offset;
	return tmp;
}

static void
read_dwarf_abbrev_32( elf_class_t *c, char *abbrev,
					  unsigned long abbrev_size )
{
	char *end;
	char *start;
	unsigned long tag;
	unsigned int bytes_read;
	unsigned long attribute;

	unsigned char *start_info;
	unsigned char *end_info;
	unsigned char *section_begin;
	unsigned int unit = 0;
	unsigned int num_units = 0;
	unsigned long length;
	int version = 0;
#if defined( DEBUG )
	unsigned long abbrev_offset;
#endif
	unsigned long offset;
	int offset_size;
	int ptr_s = 0;

#if defined( DEBUG )
	fprintf( stderr, "%s\n", c->name );
#endif

	if ( c->debug_info == NULL )
	{
		return;
	}

	start_info = c->debug_info;
	end_info = (unsigned char *) c->debug_info + c->debug_info_size;

	for ( section_begin = start_info, num_units = 0;
		  section_begin < end_info; num_units++ )
	{
		/*     fprintf(stderr,"%d %p <= %p < %p\n",num_units,start_info,section_begin,end_info);  */
		length = get_bytes( (char *) section_begin, 4 );

		if ( length == 0xffffffff )
		{
			length = get_bytes( (char *) section_begin, 8 );
			section_begin += length + 12;
			offset_size = 8;
		}
		else
		{
			section_begin += length + 4;
			offset_size = 4;
		}
	}

	if ( num_units == 0 )
	{
		return;
	}
#if defined( DEBUG )
	fprintf( stderr, "%d units\n", num_units );
#endif
	c->dbg_list =
		(debug_info_t *) sctk_malloc( num_units * sizeof( debug_info_t ) );
	c->dbg_nb = num_units;

#if defined( DEBUG )
	fprintf( stderr, "%s\n", __func__ );
#endif

	end = abbrev + abbrev_size;
	start = abbrev;
	section_begin = start_info;
	unit = 0;
	while ( (unsigned long) start < (unsigned long) end )
	{
		unsigned long entry;
		char *ptr = NULL;
#if defined( DEBUG )
		unsigned long abbrev_nb;
#endif

		entry = read_leb128( (unsigned char *) start, &bytes_read, 0 );
		start += bytes_read;
		if ( entry == 0 )
		{
			entry = read_leb128( (unsigned char *) start, &bytes_read, 0 );
			start += bytes_read;
		}
#if defined( DEBUG )
		fprintf( stderr, "entry %d\n", entry );
#endif

		tag = read_leb128( (unsigned char *) start, &bytes_read, 0 );
		start += bytes_read;
#if defined( DEBUG )
		fprintf( stderr, "tag %p\n", (void *) tag );
#endif
#if defined( DEBUG )
		fprintf( stderr, "entry %d tag %p\n", entry, (void *) tag );
#endif

		start++;

		if ( ( entry == 1 ) && ( tag == 0x11 ) )
		{
			length = get_bytes( (char *) section_begin, 4 );
			section_begin += 4;
			offset = 4;

			if ( length == 0xffffffff )
			{
				length = get_bytes( (char *) section_begin, 8 );
				section_begin += 8;
				offset = 8;
			}

			ptr = (char *) section_begin;

#if defined( DEBUG )
			fprintf( stderr, "Size %ld unit %ld\n", length, unit );
#endif

			version = get_bytes( ptr, 2 );
			ptr += 2;
#if defined( DEBUG )
			fprintf( stderr, "Version %ld\n", version );
#endif

#if defined( DEBUG )
			abbrev_offset = get_bytes( ptr, offset );
#else
			get_bytes( ptr, offset );
#endif

			ptr += offset;
#if defined( DEBUG )
			fprintf( stderr, "Abbrev_Offset %ld\n", abbrev_offset );
#endif

			ptr_s = get_bytes( ptr, 1 );
			ptr += 1;
#if defined( DEBUG )
			fprintf( stderr, "Ptr_S %ld\n", ptr_s );
#endif
#if defined( DEBUG )
			abbrev_nb = read_leb128( (unsigned char *) ptr, &bytes_read, 0 );
#endif
			ptr += bytes_read;
#if defined( DEBUG )
			fprintf( stderr, "abbrev_nb %ld\n", abbrev_nb );
#endif
		}

		do
		{
			if(!ptr)
				break;

			unsigned long form;
			char *dir;

			attribute = read_leb128( (unsigned char *) start, &bytes_read, 0 );
			start += bytes_read;
#if defined( DEBUG )
			fprintf( stderr, "attribute %02p\n", (void *) attribute );
#endif

			form = read_leb128( (unsigned char *) start, &bytes_read, 0 );
			start += bytes_read;
#if defined( DEBUG )
			fprintf( stderr, "form %p\n", (void *) form );
#endif
			if ( ( entry == 1 ) && ( tag == 0x11 ) && ( attribute != 0x1b ) )
			{
				switch ( form )
				{
					case 0:
						break;
					case DW_FORM_strp:
						ptr += offset_size;
						break;
					case DW_FORM_data1:
						ptr += 1;
						break;
					case DW_FORM_data2:
						ptr += 2;
						break;
					case DW_FORM_data4:
						ptr += 4;
						break;
					case DW_FORM_data8:
						ptr += 8;
						break;
					case DW_FORM_string:
						ptr += strlen( ptr ) + 1;
						break;
					case DW_FORM_addr:
						if ( version == 2 )
						{
							ptr += ptr_s;
						}
						else
						{
							ptr += offset_size;
						}
						break;
					default:
					{
					}
						//fprintf (stderr, "Unknown %02x\n", (unsigned int) form);
						//assert (0);
				}
			}

			if ( ( tag == 0x11 ) && ( attribute == 0x1b ) )
			{
				dir = ptr;
				if ( form == 0x0e )
				{
					dir = (char *) get_bytes( ptr, 2 );
#if defined( DEBUG )
					fprintf( stderr, "Find a indirect dir %p\n", dir );
#endif
					dir = fecth_indirect_string( c, (unsigned long) dir );
#if defined( DEBUG )
					fprintf( stderr, "Find a indirect dir %s\n", dir );
#endif
				}
				else
				{
#if defined( DEBUG )
					fprintf( stderr, "Find a comp dir %s\n", dir );
#endif
				}
				c->dbg_list[unit].dir = dir;
			}
		} while ( attribute != 0 );

		if ( ( entry == 1 ) && ( tag == 0x11 ) )
		{
			section_begin += length;
			unit++;
		}
	}
}

void sctk_read_elf_section_32( elf_class_t *c )
{
	int i;
	char *abbrev = NULL;
	unsigned long abbrev_size = 0;

	sctk_free( c->sections );
	c->sections = sctk_malloc( c->header.h32.e_shnum * sizeof( Elf_Shdr ) );

	size_t ret = 0;

	fseek( c->fd, c->header.h32.e_shoff, SEEK_SET );
	for ( i = 0; i < c->header.h32.e_shnum; i++ )
	{
		ret = fread( &( c->sections[i].h32 ), sizeof( Elf32_Shdr ), 1, c->fd );
		if( ret == 1 )
		{
			if ( ( c->sections[i].h32.sh_type == SHT_STRTAB ) && ( i == c->header.h32.e_shstrndx ) )
			{
				long off;
				sctk_free( c->string_table );
				c->string_table = sctk_malloc( c->sections[i].h32.sh_size );
				off = ftell( c->fd );
				fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
				ret = fread( c->string_table, 1, c->sections[i].h32.sh_size, c->fd );
				if( ret != c->sections[i].h32.sh_size)
					return;
				fseek( c->fd, off, SEEK_SET );
			}
		}
	}
	for ( i = 0; i < c->header.h32.e_shnum; i++ )
	{
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ), ".symtab" ) == 0 )
		{
			c->symtab_idx = i;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ), ".strtab" ) == 0 )
		{
			sctk_free( c->sym_table );
			c->sym_table = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->sym_table, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ), ".dynsym" ) == 0 )
		{
			c->dynsymtab_idx = i;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ), ".dynstr" ) == 0 )
		{
			sctk_free( c->dynsym_table );
			c->dynsym_table = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->dynsym_table, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ),
					 ".debug_line" ) == 0 )
		{
			c->debug_line_size = c->sections[i].h32.sh_size;
			sctk_free( c->debug_line );
			c->debug_line = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->debug_line, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ),
					 ".debug_info" ) == 0 )
		{
			c->debug_info_size = c->sections[i].h32.sh_size;
			sctk_free( c->debug_info );
			c->debug_info = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->debug_info, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ),
					 ".debug_aranges" ) == 0 )
		{
			sctk_free( c->debug_aranges );
			c->debug_aranges = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->debug_aranges, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ), ".debug_str" ) == 0 )
		{
			c->debug_str_size = c->sections[i].h32.sh_size;
			sctk_free( c->debug_str );
			c->debug_str = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( c->debug_str, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
		if ( strcmp( &( c->string_table[c->sections[i].h32.sh_name] ),
					 ".debug_abbrev" ) == 0 )
		{
			abbrev_size = c->sections[i].h32.sh_size;
			abbrev = sctk_malloc( c->sections[i].h32.sh_size );
			fseek( c->fd, c->sections[i].h32.sh_offset, SEEK_SET );
			ret = fread( abbrev, 1, c->sections[i].h32.sh_size, c->fd );
			if( ret != c->sections[i].h32.sh_size)
				return;
		}
	}
	/*   print_section_32(stderr,c); */
	read_dwarf_abbrev_32( c, abbrev, abbrev_size );
	sctk_free( abbrev );
}

#if defined( DEBUG )
static void
print_symbols_32( FILE *output, elf_class_t *c )
{
	int i;
	int n;
	Elf32_Shdr sec;
	Elf32_Sym *sym;
	sec = c->sections[c->symtab_idx].h32;
	sym = (Elf32_Sym *) c->symbols;

	n = sec.sh_size / sizeof( Elf32_Sym );
	fprintf( stderr, "%d symbols\n", n );
	for ( i = 0; i < n; i++ )
	{
		fprintf( output, "%2d %p-%p %s\n",
				 i,
				 (void *) sym[i].st_value,
				 ( (char *) ( sym[i].st_value ) ) + sym[i].st_size,
				 &( c->sym_table[sym[i].st_name] ) );
	}
}
#endif

void sctk_read_elf_symbols_32( elf_class_t *c )
{
	Elf32_Shdr sec;
	int i;
	Elf32_Sym *sym;
	i = c->symtab_idx;

	if((i > c->header.h32.e_shnum) || (i < 0) )
	{
		/* Do not crash if we read garbage */
		return;
	}

	sec = c->sections[i].h32;
	sctk_free( c->symbols );
	c->symbols = sctk_malloc( sec.sh_size );
	sym = (Elf32_Sym *) c->symbols;
	fseek( c->fd, sec.sh_offset, SEEK_SET );
	size_t ret = fread( sym, 1, sec.sh_size, c->fd );

	if(ret != sec.sh_size)
	{
		/* Something bad happened */
		return;
	}

	/*   read_dwarf_symbols_32(c); */

	/*   print_symbols_32(stderr,c); */
}
typedef struct
{
	char a;
	char b;
} size_2;

#define inc_line( a, n, c ) assert( ( a += n ) <= ( (char *) c->debug_line ) + c->debug_line_size )

#define DW_LNS_extended_op 0
#define DW_LNS_copy 1
#define DW_LNS_advance_pc 2
#define DW_LNS_advance_line 3
#define DW_LNS_set_file 4
#define DW_LNS_const_add_pc 8


static void
read_dwarf( elf_class_t *c, mpc_addr2line_t *ptrs, int nb )
{
	long offset;
	int j;
	char *cursor;
	char *end_of_sequence;
	long length;
#ifdef DEBUG
	long version;
	long prologue_length;
#endif
	long min_insn_length;
#ifdef DEBUG
	long default_is_stmt;
#endif
	int line_base;
	long line_range;
	long opcode_base;
	long inital_length_size;
	char *data;
	file_t file;
	char *address;
	long line;
	unsigned int bytes_read;

	char *last_address;
	long last_line = 0;
	file_t last_file = {NULL, NULL, NULL};

	int file_nb;
	int dir_nb;
	int restart_val = 0;

	cursor = (char *) c->debug_line;
	if ( cursor == NULL )
	{
		return;
	}
	data = cursor;

restart:
	address = NULL;
	offset = 4;
	inital_length_size = 4;
	file_nb = 0;
	dir_nb = 1;
	last_address = (void *) ( -1 );

	c->dir_list = realloc( c->dir_list, dir_nb * sizeof( char * ) );
	c->dir_list[0] = "./";

	length = get_bytes( cursor, 4 );
	inc_line( cursor, 4, c );

	if ( length == 0xffffffff )
	{
		length = get_bytes( cursor, 8 );
		inc_line( cursor, 8, c );
		offset = 8;
		inital_length_size = 12;
	}

#ifdef DEBUG
	version = get_bytes( cursor, 2 );
#else
	get_bytes( cursor, 2 );
#endif

	inc_line( cursor, 2, c );

#ifdef DEBUG
	prologue_length = get_bytes( cursor, offset );
#else
	get_bytes( cursor, offset );
#endif

	inc_line( cursor, offset, c );

	min_insn_length = get_bytes( cursor, 1 );
	inc_line( cursor, 1, c );

#ifdef DEBUG
	default_is_stmt = get_bytes( cursor, 1 );
#else
	get_bytes( cursor, 1 );
#endif

	inc_line( cursor, 1, c );

	line_base = get_bytes( cursor, 1 );
	inc_line( cursor, 1, c );
	line_base <<= 24;
	line_base >>= 24;

	line_range = get_bytes( cursor, 1 );
	inc_line( cursor, 1, c );

	opcode_base = get_bytes( cursor, 1 );
	inc_line( cursor, 1, c );

#ifdef DEBUG
	fprintf( stderr, "Length:                      %ld\n", length );
	fprintf( stderr, "DWARF Version:               %ld\n", version );
	fprintf( stderr, "Prologue Length:             %ld\n", prologue_length );
	fprintf( stderr, "Minimum Instruction Length:  %ld\n", min_insn_length );
	fprintf( stderr, "Initial value of 'is_stmt':  %ld\n", default_is_stmt );
	fprintf( stderr, "Line Base:                   %d\n", line_base );
	fprintf( stderr, "Line Range:                  %ld\n", line_range );
	fprintf( stderr, "Opcode Base:                 %ld\n", opcode_base );
	fprintf( stderr, "(Pointer size:               %ld)\n", offset );
#endif

	end_of_sequence = data + length + inital_length_size;

	data = cursor + opcode_base - 1;
	while ( *data != 0 )
	{
#if defined( DEBUG )
		fprintf( stderr, "%s\n", data );
#endif
		c->dir_list =
			(char **) realloc( c->dir_list, ( dir_nb + 1 ) * sizeof( char * ) );
		c->dir_list[dir_nb] = data;
		dir_nb++;
		data += strlen( data ) + 1;
	}
#ifdef DEBUG
	fprintf( stderr, "\n" );
#endif

	data++;
	while ( *data != 0 )
	{
		long val;
		c->file_list =
			(file_t *) realloc( c->file_list, ( file_nb + 1 ) * sizeof( file_t ) );
		c->file_list[file_nb].file = data;
		data += strlen( data ) + 1;

		val = read_leb128( (unsigned char *) data, &bytes_read, 0 );
		inc_line( data, bytes_read, c );
		c->file_list[file_nb].dir = c->dir_list[val];
		if ( c->file_list[file_nb].dir[0] == '/' )
		{
			c->file_list[file_nb].absolute = "";
		}
		else
		{
			c->file_list[file_nb].absolute = c->dir_list[0];
		}
#if defined( DEBUG )
		fprintf( stderr, "%s%s/%s %2d %2d\n",
				 c->file_list[file_nb].absolute, c->file_list[file_nb].dir,
				 c->file_list[file_nb].file, file_nb, val );
#endif

		read_leb128( (unsigned char *) data, &bytes_read, 0 );
		inc_line( data, bytes_read, c );

		read_leb128( (unsigned char *) data, &bytes_read, 0 );
		inc_line( data, bytes_read, c );
		file_nb++;
	}
#ifdef DEBUG
	fprintf( stderr, "\n" );
#endif
	data++;

	line = 1;
	file = c->file_list[0];
	while ( data < end_of_sequence )
	{
		unsigned char op_code;
		int adv;
		unsigned long uladv;

		op_code = *data++;
#if defined( DEBUG ) && 0
		fprintf( stderr, "OPCODE %d\n", op_code );
#endif
		if ( op_code >= opcode_base )
		{
			op_code -= opcode_base;
#if defined( DEBUG ) && 0
			fprintf( stderr, "SPECIAL OPCODE %d\n", op_code );
#endif
			uladv = ( op_code / line_range ) * min_insn_length;
#if defined( DEBUG ) && 0
			fprintf( stderr, "advance line by %d\n", uladv );
#endif
			address += uladv;
			adv = ( op_code % line_range ) + line_base;
			line += adv;
		}
		else
		{
			switch ( op_code )
			{
				case DW_LNS_extended_op:
				{
					unsigned int len;
					len = get_bytes( data, 1 );
					inc_line( data, 1, c );
					len += 1;
					op_code = *data++;
#if defined( DEBUG ) && 0
					fprintf( stderr, "EXTENDED OPCODE %d\n", op_code );
#endif
					switch ( op_code )
					{
						case 2:
							address = (char *) get_bytes( data, len - 1 - 1 );
							inc_line( data, len - 1 - 1, c );
#if defined( DEBUG )
							fprintf( stderr, "SET adresse to %p\n", address );
#endif
							break;
					}
				}
				break;

				case DW_LNS_copy:

					break;

				case DW_LNS_advance_pc:
					uladv = read_leb128( (unsigned char *) data, &bytes_read, 0 );
					inc_line( data, bytes_read, c );
					uladv *= min_insn_length;
					address += uladv;
#if defined( DEBUG )
					fprintf( stderr, "STEP adresse to %p\n", address );
#endif
					break;

				case DW_LNS_advance_line:
					adv = read_leb128( (unsigned char *) data, &bytes_read, 1 );
					inc_line( data, bytes_read, c );
#ifdef DEBUG
					fprintf( stderr, "advance line by %d\n", adv );
#endif
					line += adv;

					break;

				case DW_LNS_const_add_pc:
					uladv = ( ( ( 255 - opcode_base ) / line_range ) * min_insn_length );
					address += uladv;
#if defined( DEBUG )
					fprintf( stderr, "STEP adresse to %p\n", address );
#endif
					break;

				case DW_LNS_set_file:
					adv = read_leb128( (unsigned char *) data, &bytes_read, 0 );
					inc_line( data, bytes_read, c );
					//mpc_common_debug_error("get %d / %d", adv-1, file_nb);
					if( (adv - 1 < file_nb) && (adv > 0) )
					{
						file = c->file_list[adv - 1];
					}
					else
					{
						file.file = "";
						file.dir = "";
						file.absolute = "";
					}
#if defined( DEBUG )
					fprintf( stderr, "SET FILE %d %s %p\n", adv - 1,
							 c->file_list[adv], address );
#endif
					break;

			}
		}
#if defined( DEBUG )
		fprintf( stderr, "ADDRESS %p DIR %s FILE %s LINE %d\n", address,
				 file.dir, file.file, line );
#endif
		for ( j = 0; j < nb; j++ )
		{
			if ( ( ptrs[j].ptr > last_address ) && ( ptrs[j].ptr <= address ) &&
				 ( last_file.file == file.file ) )
			{
#if defined( DEBUG )
				fprintf( stderr, "SET ADDRESS %p DIR %s FILE %s LINE %d\n",
						 ptrs[j].ptr, last_file.dir, last_file.file, last_line );
				fprintf( stderr, "CUR ADDRESS %p DIR %s FILE %s LINE %d\n",
						 address, file.dir, file.file, line );
#endif
				ptrs[j].dir = last_file.dir;
				if ( strcmp( ptrs[j].dir, "./" ) == 0 )
				{
					ptrs[j].dir = "";
				}
				ptrs[j].file = last_file.file;

				if ( restart_val >= c->dbg_nb )
				{
					ptrs[j].absolute = "UNKNOWN";
				}
				else
				{
					ptrs[j].absolute = c->dbg_list[restart_val].dir;
				}
				TODO( "Issue in absolute prefix detection" )
				ptrs[j].absolute = "";

				ptrs[j].line = last_line;
			}
		}
		last_address = address;
		last_line = line;
		last_file = file;
	}

	cursor = data;
	if ( cursor < ( ( (char *) ( c->debug_line ) ) + c->debug_line_size ) )
	{
		restart_val++;
		goto restart;
	}
}

void sctk_read_elf_sym_32( elf_class_t *c, mpc_addr2line_t *ptrs, int nb )
{
	int i;
	int n;
	int j;
	Elf32_Shdr sec;
	Elf32_Sym *sym;
	sec = c->sections[c->symtab_idx].h32;
	sym = (Elf32_Sym *) c->symbols;

	for ( j = 0; j < nb; j++ )
	{
		ptrs[j].name = "??";
		ptrs[j].dir = "??";
		ptrs[j].file = "??";
		ptrs[j].line = -1;
	}

	n = sec.sh_size / sizeof( Elf32_Sym );
	read_dwarf( c, ptrs, nb );
	for ( i = 0; i < n; i++ )
	{
		for ( j = 0; j < nb; j++ )
		{
			void *ptr;
			ptr = ptrs[j].ptr;
			if ( ( (unsigned long) ptr >= (unsigned long) ( sym[i].st_value ) ) && ( (unsigned long) ptr <
																					 (unsigned long) ( sym[i].st_value + sym[i].st_size ) ) )
			{
				ptrs[j].name = &( c->sym_table[sym[i].st_name] );
			}
		}
	}
}
