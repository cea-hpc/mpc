======================
Write a commit message
======================

Properly format a commit message
--------------------------------

This may be one of the most important HOW-TO to write. Even if no policy is enforced about commit messages (yet), we strongly encourage people to follow these simples guidelines. A commit message is composed of 4 distinct parts:

* **A first-line** (preferably ending with a period). This should be a single the sentence, self-explanatory for the purpose of the commit, without giving to much details about it. Ideally, it should contain the component label it is changing the most (OMP, MessagePassing, Threads, Chores...) followed by a colon `:`

* **a blank line**: This is mandatory if you are going to make a multi-line commit message. Without it, Git is not able to make a difference between the "title" and "body" and will consider the whole as the "title", creating reallly ugly layouts when coming to in-depth history searches. Please, do not forget to add a blank line, it only cost an extra keypress. **A body message**, where you can detail as much as possible the reason of these changes (not a description of the changes, it would be redundant). For instance, if you resolve multiple bugs or scenarios within one commit, you may list them here

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

::

	GETOPT: simple fix for privatizing getopt vars.

	It seems that GCCs patch has difficulties not considering this getopt as system
	header as it is being included by UNISTD.h A proposed solution is to flag them
	__task by force.

	Closes #16
	
::

	And remind to stay as far as possible from commit message like these (from MPC
	framework history itself):

::

	-> Many small fixes
	-> Remove bug
	-> Add new test
	-> Dummy Commit
	-> Hello World works
	-> Update (multiple times in a row !!!)
	...
