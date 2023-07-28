## General information

* **Release version**: <X.Y.Z>
* **Reference commit**: <SHA>
* **Release Author**: <Name> (GPG ID if relevant)

This MR is intended to help releasing a new version of PCVS.

## Pre-actions

* [ ] The pipeline succeeds
* [ ] Changelog has been updated accordingly
* [ ] Test Coverage is above 50% (to be raised)
* [ ] Documentation can be generated through `tox -e lint-docs`
* [ ] Version number is up-to-date (`pcvs/version.py` & `docs/source.conf.py`)
* [ ] Any newly added non-python files is added to `MANIFEST` if installation is
  required

## Package Validation

1. [ ] Build the package for official repo:
    * `pip3 install build twine`
    * `python3 -m build`
    * `python3 -m twine check dist/*`
2. [ ] Upload to **TEST** PyPI database: `python3 -m twine upload --repository
   testpypi dist/*`
3. [ ] Install from this test database (Docker recommanded):
    * `python3 -m pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pcvs==X.Y.Z`
4. [ ] Documentation can be built by [ReadTheDocs](https://readthedocs.org)

### Post-release

*Before closing this issue, the last steps in the following will detail how to
set the project back to a "development" mode:*
* [ ] Upload package distribution to official PyPI database:
    * `python3 -m twine upload dist/*`
* [ ] Create a tag `vX.Y.Z` & push the tag to repos (Gitlab & Github)
* [ ] Generate new archive & upload it to the website
    * `version=X.Y.Z git archive --format=tar.gz --prefix=pcvs-${version}/ v${version} > pcvs-${version}.tar.gz`
    * https://pcvs.io/download/
* [ ] After-merge, push the master to repos (Gitlab & Github)
* [ ] Bump PCVS version to `X.Y.(Z+1)dev0`

