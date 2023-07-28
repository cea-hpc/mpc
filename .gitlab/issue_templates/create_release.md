## General information

* **Release version**: <X.Y.Z>
* **Reference commit**: <SHA>
* **Release Author**: <Name> (GPG ID if relevant)
* **Affiliated MR**: <MR ID>

This issue and any related MR must have the "Release Process" label. For a
better involvement of anyone in the process, the author should pick up voluntary
"lieutenants", to spread the workload and shorten the time to release (=*divide
and conquer*).  For instance, the verification checkboxes below should be
distributed over the development team.

Please note that, considering the MPC framework project and the active
development team size, a release procedure should not take longer than 3 weeks
to complete (all verifications included).

## Verifications

This Release will follow the [General
Recommendations](MPC/mpc-release-procedure) to conduct the process. For this
purpose, the reference commit shown above will have to be checked against the
following topics. A possible way is to assign a group of tasks to a lieutenant,
leading its completion. **Each topic** leader should add a **new comment** to
allow contributors to participate and follow the process. To keep contributions
packed by topic, **please use the `Reply` button** (depicted as a bubble). This
will create as many discussion threads as necessary to easily follow
verification progress. All these topics and items are not exhaustive and can be
extended upon request, as needed.

### Project build

*This topic will ensure the MPC framework is still able to be built in a large
number of ways, including support for:*
* [ ] Privatization (use of `-fmpc-privatize`, GCC & Binutils, dyn-priv...)
* [ ] Process mode (`--enable-process-mode` when building)
* [ ] Intel support
* [ ] Fortran compatibility (at build time & run-time re-generation)

### Project Run

*This topic will ensure the MPC framework is still able to run (from a functional
perspective), including:*
* [ ] Running a simple program, containing MPI, OpenMP and both
* [ ] Checking the Gitlab pipeline to be "all-green"
* Checking multiple run configurations, at least each of the following:
   * [ ] thread-based (=a single UNIX process) -> `-n=X -p=1`
   * [ ] process-based (=as many UNIX process as MPI ranks) -> `-n=X -p=X`
   * [ ] Hybrid-based (=a mix from scenarii above) -> `-n=X -p=Y` where `X > Y > 1`
   * [ ] Multiple thread-models (`-m=X`), especially `pthread` and `ethread_mxn`
   * [ ] Multiple networks (at least Infiniband & SHM)
   * [ ] Core oversubscription (=more MPI ranks than core per process) -> `-n=X
     -p=Y -c=Z`, where ` X/Y > Z`

### Performances

*This topic will ensure the MPC framework is still conformant with its
perforamnce standard. An in-depth nalysis of performance assessment should be
conducted to evaluate potential regression from the previous release. This
evaluation should at least include:*
* [ ] MPI performance (through benchmarks like IMB & proxy apps)
* [ ] OpenMP performance
* [ ] Simple privatization performance hit

### Chores

*This topic will ensure the release process is going forward, and will prepare
the actual archive to be released. This should at least include:*
* [ ] Ensure version number is correct in `./VERSION`
* [ ] Generating the changelog, from the previous release (`git log`)
* [ ] Add the Changelog to the repository
* [ ] Generating the archive (through the `tools/gen_archive` script)
* [ ] Checking the tarball once created (in Docker env, preferably)
* [ ] Creating and pushing a new tag for this release (check [the
  guidelines](MPC/Branches-and-tags#tags)). Check its availability under
  `Repository > Tags`. You may edit the tag to add the Changelog to it.
* [ ] Uploading a new archive onto the website.
* [ ] Writing down a new entry on the
  [Download](https://mpc.hpcframework.com/download) and adding a new post
  (=news) about it.

### After-Release steps

*Before closing this issue, the last steps in the following will detail how to
set the project back to a "development" mode:*
1. [ ] Merge the MR for this release into the development branch. (=fast-forward)
2. [ ] Add or check the changelog for the release under Gitlab.
3. [ ] Set "Allowed to Merge"  to "Maintainers" for the development branch
   (maintainers only).
4. [ ] Close this issue.

## Final recommendations

To give your acceptance to the forthcoming release, you are requested to
`/approve` this MR. The release is eligible as soon as this MR receives:
* **5 approvals overall**
* **At least 1 approval from the technical leader(s) of each organization
  actively contributing to the project**.

@all developers are highly encouraged to keep track of this procedure. Any merge
into the development is interrupted until the release process is completed. Any
branch will have to be updated (rebase/merge) onto this released branch
afterwards.
