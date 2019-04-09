
man1_files=$(wildcard man/man1/*.1in)
man1_generated=$(man1_files:man/man1/%.1in=$(M_PWD)/BUILD_MPC_Doc/man/man1/%.1)
man3_files=$(wildcard man/man3/*.3in)
man3_generated=$(man3_files:man/man3/%.3in=$(M_PWD)/BUILD_MPC_Doc/man/man3/%.3)
man5_files=$(wildcard man/man5/*.5in)
man5_generated=$(man5_files:man/man5/%.5in=$(M_PWD)/BUILD_MPC_Doc/man/man5/%.5)

all: setup_dirs $(man1_files) $(man1_generated) $(man3_files) $(man3_generated) $(man5_files) $(man5_generated)

$(man1_generated):$(M_PWD)/BUILD_MPC_Doc/man/man1/%.1:man/man1/%.1in
	@./build_manpage $^ > $@
$(man3_generated):$(M_PWD)/BUILD_MPC_Doc/man/man3/%.3:man/man3/%.3in
	@./build_manpage $^ > $@
$(man5_generated):$(M_PWD)/BUILD_MPC_Doc/man/man5/%.5:man/man5/%.5in
	@./build_manpage $^ > $@

setup_dirs:
	@mkdir -p $(M_PWD)/BUILD_MPC_Doc/man/man1
	@mkdir -p $(M_PWD)/BUILD_MPC_Doc/man/man3
	@mkdir -p $(M_PWD)/BUILD_MPC_Doc/man/man5

.PHONY: setup_dirs 

clean:
	@rm -f $(M_PWD)/BUILD_MPC_Doc/man/man1
	@rm -f $(M_PWD)/BUILD_MPC_Doc/man/man3
	@rm -f $(M_PWD)/BUILD_MPC_Doc/man/man5
