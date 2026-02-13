======================
Write a commit message
======================

Properly format a commit message
--------------------------------

This may be one of the most important HOW-TO to write. Even if no policy is enforced about commit messages (yet), we strongly encourage people to follow these simples guidelines. A commit message is composed of 4 distinct parts:

* **A first-line** (preferably ending with a period). This should be a single the sentence, self-explanatory for the purpose of the commit, without giving to much details about it. Ideally, it should contain the component label it is changing the most (OMP, MessagePassing, Threads, Chores...) followed by a colon `:`

* **a blank line**: This is mandatory if you are going to make a multi-line commit message. Without it, Git is not able to make a difference between the "title" and "body" and will consider the whole as the "title", creating really ugly layouts when coming to in-depth history searches. Please, do not forget to add a blank line, it only cost an extra keypress. **A body message**, where you can detail as much as possible the reason of these changes (not a description of the changes, it would be redundant). For instance, if you resolve multiple bugs or scenarios within one commit, you may list them here

* **Gitlab-related anchors**, to interact directly from commit messages with issues and merge-requests. Not mandatory but highly encouraged to do so, as Gitlab can then generate a bidirectional link from commit to content. It refers to them by their number, prefixed with a `#` for issues and `!` for merge-requests. It is even possible to close an issue from a commit message by using keywords ! for instance, if a commit message contains:

  - `#23`: issue #23 is mentioned by this commit and will appear in issue
    description.

  - `!14`: merge-request is mentioned by this commit and will appear in
    merge-request description.

  - `Closes #23`: close issue #23 (works also with `Fix|Resolve`). The ticket
    will be automatically closed when this commit will be merged to the default
    project branch (probably the devel, after a merge-request). Do not expect to
    see the ticket closed as soon as you pushed it!

More information here: https://docs.gitlab.com/ee/user/project/issues/crosslinking_issues.html

Clearly, it is asking for a bit of work to write a good commit. But this is one of the best way to ease the reviewing process by explaining to others what you did, how you did it and why. Even more, by taking some time to summarize your work, you may discover some pitfalls or even better solutions which did not come to your mind the first time. Here is a good example of what a commit message should be:

.. code-block:: text

	GETOPT: simple fix for privatizing getopt vars.

	It seems that GCCs patch has difficulties not considering this getopt as system
	header as it is being included by UNISTD.h A proposed solution is to flag them
	__task by force.

	Closes #16

And remind to stay as far as possible from commit message like these (from MPC framework history itself):

.. code-block:: text

	-> Many small fixes
	-> Remove bug
	-> Add new test
	-> Dummy Commit
	-> Hello World works
	-> Update (multiple times in a row !!!)
	...

Make use of the pre-commit
--------------------------

`pre-commit`_ is a python tool to run automated tasks on commit.
It helps keep the codebase uniform and prevents simple mistakes to be incorporated.
The use of this tool is strongly recommended !

.. _pre-commit: https://pre-commit.com/

Installation of the tool is a simple pip installation

.. code-block:: sh

   $ pip install pre-commit
   # Alternatively, pre-commit is available in spack
   $ spack install py-pre-commit

Then, the setup is a simple command launch in the MPC root directory

.. note::

   Alternatively, you can use `prek`_ which is a Rust rewrite of pre-commit and is fully compatible

   .. code-block:: sh

      # using pip
      $ pip install prek
      # using cargo
      $ cargo install --locked prek

.. _prek: https://prek.j178.dev/

.. code-block:: console

   $ pre-commit install

It is configured in `.pre-commit-config.yaml` and runs the following hooks on commit

* **executables have shebangs**: checks that the executables scripts begin with a shebang
* **merge conflicts**: checks that no conflict marks are present in the commit
* **don't commit to branch**: prevents a commit to be directly appended to `devel` or `master`
* **python ast**: checks that python scripts are syntactically correct
* **json**: checks that JSON files are syntactically correct
* **yaml**: checks that YAML files are syntactically correct
* **fix end of files**: checks that files doesn't end with a newline
* **trailing whitespace**: trims the trailing whitespaces
* **typos**: runs the typos-cli tool which is a spell-checker looking for typos in the commit
* **clang-tidy**: runs clang-tidy to lint common mistakes
* **uncrustify**: check if the code is well formatted according to the `uncrustify.cfg` configuration
* **git blame**: updates the git configuration to ignore some commits from `git blame`

To commit without the pre-commit, you shall use the ``--no-verify`` option

.. code-block:: console

   $ git commit --no-verify

Be aware that the pre-commit is present in the CI and code on which it is failing cannot been merged into the devel branch.

Failing pre-commit
^^^^^^^^^^^^^^^^^^

In most cases, the reason of the failing is self-explanatory.
Some failures require manual intervention to be resolved.

Common
''''''

The ``fix end of files`` and ``trailing whitespace`` hooks modify files upon errors found.
You can verify that the changes aren't error prone

.. code-block:: console

   $ git diff

The changes should be quite trivial. You can than stage the modified files and commit as usual.

typos
'''''

To avoid false positive being introduced, the ``typos`` hook does not automatically write on detected error.
It prints the error and the proposed solutions.
In case, there is only one suggested correction, you can automatically fix it by running

.. code-block:: console

   $ typos -w <path/to/file>

In case ``typos`` is unable to find an unique solution, you will have to correct the files yourself.

You can then stage the modification and commit as usual.

In case ``typos`` reports a false positive, please refer to `False positive from typos`_.

uncrustify
''''''''''

To avoid undesired formatting, the ``uncrustify`` hook does not automatically reformat the files.
You have to run it manually

.. code-block:: console

   $ uncrustify -c uncrustify.cfg -f <path/to/file> --replace

Then, it advised to check the formatting as it can be a bit strange sometimes.
Finally, you stage and commit your changes as usual.

Troubleshooting
^^^^^^^^^^^^^^^

clang-tidy cannot found compile_commands.json
'''''''''''''''''''''''''''''''''''''''''''''

``clang-tidy`` requires a ``compile_commands.json`` file to provide diagnostics.
To generate this file, you need ``bear``.
The easiest way is to use the ``installmpc`` script with the ``--enable-lsp`` option to generate the ``compile_commands.json`` file.

False positive from typos
'''''''''''''''''''''''''

Sometimes, errors reported from ``typos`` aren't misspelling.
The easiest way to fix those is to rename the faulty word to something else to avoid collision with a misspelled word.

Alternatively, you can add the desired word to the dictionary to be able to use it anyway.
To do so, please modify the ``utils/.typos.toml`` file.
You need to add the following

.. code-block:: toml

   [default.extend-words]
   your-word = "your-word"

It tells ``typos`` that whenever it encounters ``your-word`` it should correct the spelling by ``your-word``

Failing pipeline on passed pre-commit
'''''''''''''''''''''''''''''''''''''

A common reason for this to happen, is a mismatch between the local version of the tools used and the one used by the runner.

Please consider checking you have the same version as the CI:

- ``pre-commit``: 4.2.0
- ``clang-tidy``: 18.1.3
- ``typos``: 1.36.2
- ``uncrustify``: 0.80.1
