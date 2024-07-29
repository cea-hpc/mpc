==========================
Branch & tag management
==========================

General Description
===================

To fully understand what a branch or a tag is, please refer to the `Git documentation <https://git-scm.com/doc>`. Here, we want to describe how branches and tags grow, based on team guidelines.

Branches
--------

Naming
^^^^^^
There is absolutely no restriction on what your **local** branch names are. But, to ensure people can get a quick idea of the content of a reference just by its name, we encourage (for now, maybe one day could be enforced) people to name their branch they submit onto the Gitlab server after the following:

* The name should contain the purpose of the branch. Is it relative to a bug, a feature, documentation, testing, a "poqcad" (stands for "Proof of Concept / Quick and Dirty").

* The MPC component related to the branch. Is it related to OpenMP, MPI, test-suite, Accelerators, RPCs, privatization...?

* An up-to-three-word precision of what the branch is actually focused on. For instance, OpenMP task dep fixes, MPI collective optimization, integration of new tests?

* **A merge-request branch has to be prefixed with `mr-`**

A branch name should be constructed as follows:

.. code-block:: sh

	<purpose>-<component>-<precision>

Here is an example of a well-formatted branch name:


.. code-block:: sh

	# Contains fixes relative to OpenMP task dependencies 
	bug-omp-taskdep-fixes
	# Contains feature for new MPI collectives optimizations by hardware 
	feat-mpi-collopt-bxi
	# Contains installmpc modifications 
	backend-install-fixes
	# Contains a new feature to disable automatic privatization 
	feat-priv-disable-autopriv


What should be decided is to gather under a single namespace branch related to the same purpose, i.e using `mr/` prefix instead of `mr-`. While being transparent, it allows better bulk operations.


Tags
----

A tag is an immutable reference to a commit. It is an alias to commit in the project history. Public tags should only be pushed onto Gitlab for release purposes. Anyone can create his own tags but none of them should be pushed to the mainframe. To create a new tag:

.. code-block:: sh

	$ git tag <tagname>
	# to delete an existing tag
	$ git tag -d <tagname>


A better way to create a tag is to "annotate" them, by leaving a comment inside saying what this tag should represent. For instance, a release tag may contain the complete changelog from the previous version:

.. code-block:: sh

	$ git tag mytag -a -m "Really important tag"

The best way to create a release tag is also to sign the tag, enforcing a high level of trust (as long as people trust the tag author). Tags can then be verified to ensure it. As a reminder, without signatures, anyone can take someone else's identity with Git (just a configuration field). As for any content made on your behalf, you should use GPG-signature as much as possible. If you properly configured a GPG key in your `~/.gitconfig`, you may just run:

.. code-block:: sh

	git tag -s -a MPC_X.Y.Z -m "New MPC Release X.Y.Z, including:

	* Feature A
	* bugfixes B
	* ...
	"

HOW-TOs
=======

Create a new, personal, local branch
------------------------------------

.. code-block:: sh

	# forked from the current commit (the one currently checked out)
	$ git branch mybranch
	# move to that new branch
	$ git checkout mybranch


Push my work onto the Gitlab server
-----------------------------------

.. code-block:: sh

	#considering SSH protocol
	$ git push git@gitlab.paratools.com:cea/mpc.git mybranch
	# if you clone from Gitlab
	$ git push origin mybranch
	# to add a new remote server and set it up with "gitlab" alias
	$ git remote add gitlab git@gitlab.paratools.com:cea/mpc.git
	$ git fetch gitlab


**Once a branch is pushed to Gitlab, it CANNOT be rewritten !**. Please consider
to ban using the following on commits pushed to Gitlab:

* the `--amend` argument to `git commit`

* the `--force` argument to any Git command

* `git reset --hard <ref>` with ref being part of an already pushed history

* `git rebase <ref>` with ref being part of an already pushed history

While this behavior is enforced for critical branches (devel, master, pt\_devel and some merge-requests branches), the server allows branches to be push-forced, but at your own risk (limited to personal/in-progress work). People checking out branches from Gitlab has to be aware that some branch can be rewritten to allow such flexibility. In the meantime, any mainline is guaranteed to remain consistent, *no matter what*. 

Erase my branch from the remote server
--------------------------------------

To remove your personal branch from Gitlab servers, one could run:

.. code-block:: sh

	git push gitlab :mybranch

**CAUTION: THIS CANNOT BE UNDONE FROM SERVER-SIDE !**.

Note the destructiveness of the operation and use it with precaution. For obvious reasons, some branches cannot be deleted. Also, note that merge-requests branch will (should) be automatically removed after the merge. No manual branch deletion is required afterward.


Create a new MPC release tag and push it to Gitlab
--------------------------------------------------

To push a tag, just consider it as a regular branch name, Git will do the rest:

.. code-block:: sh

	$ git tag -s -a MPC_X.Y.Z -m "New MPC Release X.Y.Z, including:

	* Feature A
	* bugfixes B
	* ...
	"
	$ git push gitlab MPC_X.Y.Z