======================================
Request for merging work into mainline
======================================

General description
===================

In order to publish changes, it is now required to do it through Merge requests (also known as Pull requests, the Github equivalent). Nobody is allowed to push directly into the default branch. Only merge requests can be approved by maintainers to do so. This enforcing will guarantee a minimal amount of regression that could be integrated into the codebase. Merge requests are integrated to the mainline through the following rules:

* The branch to merge has to be up-to-date with the mainline. It means that any commit belonging to the devel branch **has to** be in your merge-request branch. Doing either a merge or a rebase to do so is up to you. **Gitlab does not allow to merge diverging branches**. For more information, see the :ref:`section<prepare>` below. The merge will *fast-forward* the main branch reference to the HEAD of your merge branch. It means there is not merge here (despite the name). No "loop" will be created and a strict linear history is enforced (for now). Maybe this workflow will have to be updated in order to merge multiple complex and diverging work. If unsure about this process, please read :ref:`the dedicated section<prepare>`.

* A pipeline is run for each submitted merge-request, as soon as a commit is pushed onto the source branch. Please do not abuse it by submitting one commit one hundred times a day. Prefers pushing less regularly multiple commits at once (but you can still push every commit for common branches, to save your work).

* When someone posts a new comment to the merge-request, a "thread" is created. To merge the branch, all opened threads have to be marked as "resolved" the the merge to be possible. Otherwise, maintainers won't have access to the "merge" button.

* By adding `WIP:` to the merge-request title, you can let people know this MR is under progress and not intended to be reviewed or merged now. If so, the "merge" button is hidden until such a token is removed from the title. Still, a pipeline is regularly run on it.

* Even if only maintainers are eligible to "click" on the Merge button, the validity of changes is a team decision and any active contributor can review changes and either approve them or ask for changes (through comments). If changes seems valid to you, post a single comment containing `/approve` and you'll be registered as one of the approvers. Once a Merge-request reaches **three** approvals, the MR is eligible for merge and a maintainer can proceed to do so (not before...). For now, there is no plan to support a `/reject` command. The equivalent would be a properly-written, justified and documented comment, convincing the author (or maintainers) that MR is invalid and should be closed.

* **A merge-request branch should be prefixed by `mr-`**. It then implies some protection against force-push and merge/push rules. Not conformant MR might be closed.

HOW-TOs
=======

.. _prepare:

Prepare the branch to be merged
-------------------------------

As said, MPC workflow expects branch to be merged to be as linear as possible. No merge-point are 'allowed' within a branch, to ease debug sessions. Once your work is open to be merged in a target branch (considering the devel branch here but applicable to any branch), you may create a branch prefixed by `mr-`, this pattern is used. This is highly encouraged to:

* easily recognize MR branches (bulk management)
* automatically apply protections to these MR branches

For a branch to be allowed to be merged into the main branch, it **has to** contain the main branch history. In other words, the last commit of the devel branch (at the time of your merge) **has to** be in your own branch. Because the main branch can evolve (i.e. new commits) while working on your own, you'll have to synchronize the MR branch at one point. The process we are going to describe here is to apply at any time you want to propagate in your own branch the content of the main branch, in order to sync them before the merge (or just to
keep it up-to-date to avoid later issues). Let consider your branch is labeled `mr-A`:

1. In your local repository, download the last version of the mirror. Considering    the Gitlab to be origin: `git fetch origin --prune`. The `--prune` allows *mirrored* branches deleted from the server to be deleted from your local copy. **This does not apply to local branches**. For instance,if your locally checked out the "devel" branch, it won't make it disappear. But if the server-side devel branch is deleted, your local `origin/devel` will.

2. Now, your local repo is in sync, you may proceed to update your branch. The preferred approach is to use a **rebase** because it keeps the history clean in your own branch, without creating any merge loop. To rebase the `mr-A` on top of the just-sync server-side devel branch:

.. code-block:: sh

	git checkout mr-A
	git rebase`origin/devel

2. (bis) In some *rare* scenarios, it may not be convenient to rebase the branch. Mainly this can occur after you try to rebase it and it comes with **a lot** of conflicts. The main pattern is because your branch modify a set of lines multiple times (once per commit for instance) and the devel branch modified it in the meantime. When rebasing,**each commit** may create a conflict, leading to a rebasing nightmare. In that type of scenario *only*, and with the approval of the reviewing peers/maintainers, a punctual merge can be done. To do so, instead of the rebase, run the following command. You will have to resolve conflicts only
once (only the last versions of devel & mr-A branch will be compared):

.. code-block:: sh

	git checkout mr-A
	git merge origin/devel

Please note that this will create a merge loop, a thing MPC wants to avoid. Please note that only the rebase can be used to deploy the `mr-A` branch to the main branch, this step is done by Gitlab when approving the MR. For more explanation about merge and rebase, `you can check the official git documentation <https://git-scm.com/book/en/v2/Git-Branching-Rebasing>`_

A final word about the `git pull` option. This command is just a 'script' applying `git fetch` + `git merge` (considering no particular Git configuration in `.gitconfig`). Please use with care when syncing a branch to be merged.

Create a new merge-request
--------------------------

To create a merge-request, you **have to** have:

* A branch with your changes named `mr-*`, to be conformant with Gitlab semantics

* a clear title for your MR, stating what it is about.

* Read the template available on gitlab. You should copy it. Please fulfill all the required fields in order to make requests ready for people to review it. It is mandatory to follow the template, at the risk of getting a Bot warning. The bot only checks for section headers.

* checked MPC is still able to build and run correctly

Once these prerequisites accomplished, you may open an MR. To do so: `Merge-Request > New Merge Request`. The source branch is the one to merge, where the changes are located (i.e. the `mr-*` branch). The target branch is the branch to merge with (where changes will go). You may want to select the `pt_devel` branch here. Then, here are the steps to fill it correctly:

* As for tickets, choose an appropriate title, able to describe in one sentence the content of this pull-request

* Pick up as assignee the one most suitable to take care of this merge request. In case of doubt, leave it blank.

* As for tickets, choose the appropriate milestone and labels

* Two options are shown:

	* `Delete source branch when the merge request is accepted`: should be checked by default. This is good practice do delete the branch just merged from the Gitlab repository, to avoid old stalled branches.

	* `Squash commits when the merge request is accepted.`: Up to the submitter to decide if all commits contained is the request should be squashed into a single commit before being merged. The committer ownership will be transferred to the user processing the merge (not necessarily the author, who keep the author ownership)

* Then, click `submit the merge request`. The bot will then take care of telling you if you made it correctly or not.

**Side note:** As an alternative to creating an MR, when pushing commits to a branch (named `feature` for instance), a message in `git push` offers you a one-click link to create the merge-request from this branch to the default one :


Contribute to an existing merge-request
---------------------------------------

The reviewing process
-------------------------

Once a merge-request is submitted and tagged with `Ready for Review` labels, it is time for developers to review changes to be merged with the mainline. It is their only chance to give their point/advice about them before they become definitive from a project history perspective.

To review changes, click on the `Changes` tab and browse through the UI. You can also check the branch out and go in-depth through the command line (recommended for archive, not supported by Gitlab UI). Please check also pipeline task status, especially if anyone comes red. **Take also some time to check the branch history, to detect any merge loop or severe GIt misusage**. History readability and maintenance also depends on it.

For each question/comment, you may have, post a comment. You can also answer to a previous comment by using the 'note' icon ("Reply" button). This will put your answer in a dedicated thread, easier to maintain (fold/unfold).

You may also add put a comment on a specific changes line. This will encourage suggestions. To do so, click on the bubble on the left of the line to comment (in `Changes` tab), it will open a textbox for your comment. As Gitlab will extract a chunk of lines to display (the commented one + 5-10 lines **above**), add your comment to the **last line** to highlight for your comment to render properly. Additionally, you may add suggestions (=proposed changes). To do so, highlight the code to comment with the mouse and then click on the comment bubble. An edition icon like a "prompt" should appear, allowing you to insert a code block. Put in that code block what should replace the code selection.

Once satisfied with changes/answers, you may approve the merge request by posting a comment with `/approve` as the only message. You are now registered as an approver of the MR. Once the MR reaches the required amount of approvals, the merge-request will be eligible for merge. Please note that Merge-request authors cannot approve their own merge-request. Also **once approval is given, it is not possible (=complex) to un-approve it** (no `/unapprove` command). Be sure to agree with submitted changes before approving.

Merge a new merge-request
-------------------------

The branch to merge has to be up-to-date with the default branch (through a merge or a rebase) to ensure the absence of conflicts. Otherwise, the request process will complain about it. Then, a pipeline will be run to ensure there wouldn't be any regression once merged. In the meantime, any developer can review the code and potentially addressing comments or concerns. Any comment should be answered before the process to continue (each comment will open a new thread and will ask for its completion before going further).

After all, comment marked as resolved AND an "all-green" pipeline, maintainers can start the merging process. The merge commits message should be edited to remove the `"Merge branch.... into..."` and replace it with something more clear. Note that a good convention is to start the commit message with `MERGE:` for consistency. Then click on the "merge" button.
