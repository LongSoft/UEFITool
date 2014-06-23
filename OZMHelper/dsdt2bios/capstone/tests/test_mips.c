/* Capstone Disassembler Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013> */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <capstone.h>

struct platform {
	cs_arch arch;
	cs_mode mode;
	unsigned char *code;
	size_t size;
	char *comment;
};

static csh handle;

static void print_string_hex(char *comment, unsigned char *str, int len)
{
	unsigned char *c;

	printf("%s", comment);
	for (c = str; c < str + len; c++) {
		printf("0x%02x ", *c & 0xff);
	}

	printf("\n");
}

static void print_insn_detail(cs_insn *ins)
{
	cs_mips *mips = &(ins->detail->mips);

	if (mips->op_count)
		printf("\top_count: %u\n", mips->op_count);

	int i;
	for (i = 0; i < mips->op_count; i++) {
		cs_mips_op *op = &(mips->operands[i]);
		switch((int)op->type) {
			default:
				break;
			case MIPS_OP_REG:
				printf("\t\toperands[%u].type: REG = %s\n", i, cs_reg_name(handle, op->reg));
				break;
			case MIPS_OP_IMM:
				printf("\t\toperands[%u].type: IMM = 0x%"PRIx64 "\n", i, op->imm);
				break;
			case MIPS_OP_MEM:
				printf("\t\toperands[%u].type: MEM\n", i);
				if (op->mem.base != X86_REG_INVALID)
					printf("\t\t\toperands[%u].mem.base: REG = %s\n",
							i, cs_reg_name(handle, op->mem.base));
				if (op->mem.disp != 0)
					printf("\t\t\toperands[%u].mem.disp: 0x%" PRIx64 "\n", i, op->mem.disp);

				break;
		}

	}

	printf("\n");
}

static void test()
{
//#define MIPS_CODE "\x8f\xa2\x00\x00"
//#define MIPS_CODE "\x00\x00\xa7\xac\x10\x00\xa2\x8f"
//#define MIPS_CODE "\x21\x30\xe6\x70"	// clo $6, $7
//#define MIPS_CODE "\x00\x00\x00\x00" // nop
//#define MIPS_CODE "\xc6\x23\xe9\xe4"	// swc1	$f9, 0x23c6($7)
//#define MIPS_CODE "\x21\x38\x00\x01"	// move $7, $8
#define MIPS_CODE "\x0C\x10\x00\x97\x00\x00\x00\x00\x24\x02\x00\x0c\x8f\xa2\x00\x00\x34\x21\x34\x56"
//#define MIPS_CODE "\x04\x11\x00\x01"	// bal	0x8
#define MIPS_CODE2 "\x56\x34\x21\x34\xc2\x17\x01\x00"

	struct platform platforms[] = {
		{
			.arch = CS_ARCH_MIPS,
			.mode = CS_MODE_32 + CS_MODE_BIG_ENDIAN,
			.code = (unsigned char *)MIPS_CODE,
			.size = sizeof(MIPS_CODE) - 1,
			.comment = "MIPS-32 (Big-endian)"
		},
		{
			.arch = CS_ARCH_MIPS,
			.mode = CS_MODE_64+ CS_MODE_LITTLE_ENDIAN,
			.code = (unsigned char *)MIPS_CODE2,
			.size = sizeof(MIPS_CODE2) - 1,
			.comment = "MIPS-64-EL (Little-endian)"
		},
	};

	uint64_t address = 0x1000;
	cs_insn *insn;
	int i;

	for (i = 0; i < sizeof(platforms)/sizeof(platforms[0]); i++) {
		cs_err err = cs_open(platforms[i].arch, platforms[i].mode, &handle);
		if (err) {
			printf("Failed on cs_open() with error returned: %u\n", err);
			continue;
		}

		cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

		size_t count = cs_disasm_ex(handle, platforms[i].code, platforms[i].size, address, 0, &insn);
		if (count) {
			printf("****************\n");
			printf("Platform: %s\n", platforms[i].comment);
			print_string_hex("Code:", platforms[i].code, platforms[i].size);
			printf("Disasm:\n");

			size_t j;
			for (j = 0; j < count; j++) {
				printf("0x%"PRIx64":\t%s\t%s\n", insn[j].address, insn[j].mnemonic, insn[j].op_str);
				print_insn_detail(&insn[j]);
			}
			printf("0x%"PRIx64":\n", insn[j-1].address + insn[j-1].size);

			// free memory allocated by cs_disasm_ex()
			cs_free(insn, count);
		} else {
			printf("****************\n");
			printf("Platform: %s\n", platforms[i].comment);
			print_string_hex("Code:", platforms[i].code, platforms[i].size);
			printf("ERROR: Failed to disasm given code!\n");
		}

		printf("\n");

		cs_close(&handle);
	}
}

int main()
{
	test();

	return 0;
}
