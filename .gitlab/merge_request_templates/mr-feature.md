## Type of change: New Feature

**This MR intends to add new functionalities without changing any existing
features.**

## Prerequisites
I ensure the proposed changes:
- [ ] are up-to-date with the default branch (currently `devel`)
- [ ] do not break basic MPC build process: `./installmpc --prefix=$PREFIX`
- [ ] can still run a simple MPI `main()`
- [ ] have been tested under multiple configurations, in respect of C & Fortran
  compatibilities + with & without privatisation.
- [ ] are properly documented thanks to Doxygen (or at least in-place
  documentation) and Markdown (in `*.md` for major modifications)

## Description

Please describe here what changes are about.

## Method to validate this Merge-Request

Please indicate how this merge-request can be tested and provide any test-suite
path (ex: `OpenMP/GOMP/libgomp.c/new-dir`) or attach to this request any test
file to be included for further test-suite release. **As this is a feature MR,
it should be required to to provide ways to ensure this feature is functional.**
While the non-regression is covered through the pipeline process, any new
feature may not be covered:
* Provide a link or direct results embedded into this MR (attached files).
* Consider adding such to the pipeline (if relevant).

## Consequences to users

List here potential consequences users may have with these changes (new program,
a new option, new config entry). This section will help to write Changelog when
MPC releases.

## Consequences to developers

List here what these changes will result in the project sources. List only major
impact (like moving an important script, a whole directory, changing things that
would need to rebuild from scratch, like Makefile impacts...). Use this as
lightweight documentation of your changes

## Related tickets

If any, list tickets related, mentioned and/or even closed by this
Merge-request. To list a ticket using the `#` syntax. For instance:
- Related: #2
- Closes: #4
