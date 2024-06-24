========================
Interact with Gitlab Bot
========================

General Description
===================

A bot has been added to this actual Gitlab platform, to automate part of the process of reviewing code and asses its quality. Currently the bot tracks:

* Issue opening/closing/updating

* Merge-request opening/closing/updating

* Merge-request comments

Issue
-----

When a new issue is opened, if the `Bug` label is attached to it, the bot will ensure the description follows established guidelines listed [here](GL/issues-tickets#create-a-new-bug-report). If not, a message indicates steps to comply with these rules is posted as the first comment. You should review it and fix what necessary. Basically, if you just copy/paste the template, it should be fine. If not, please refer to the [FAQ](#faq) below for help.

The `Wrong Format` may be attached to the opened issue to notify you (and anyone contributing to this project) that the issue does not comply with the rules and should be fixed before letting this issue reviewed. **An issue not fixed in a reasonable time may be closed without further notice until an update is made on the description**.

Any other issue is ignored, not part of the current process. There is (for now) no other guidelines for issues. One that could come soon is an analysis of currently set labels, to ensure these are properly used depending on the context of the issue. Please choose only one label from each as described [here](GL/milestones-and-labels#labels)

Merge-Request
-------------

When a merge-request is opened, its description is analyzed to check template compliance, as described [here](GL/merge-requests). If not, a comment indicates steps to follow in order to fix it. In the meantime, a `Wrong Format` label is used to taint it for other developers as non-conformant-yet. Still, Copy/paste the template should be fine (available [here](GL/merge-requests#create-a-new-merge-request)). 

As explained in the same documentation, a merge-request is reviewed and subject to approvals by peers before being merged into the main branch. This process is supported by the `/approve` command, posted as a regular comment. If the comment only contains the command, the approval is registered but the comment is removed. In that context, the bot keeps track of the number of approvals, posting a new comment with an update each time a new approval is submitted. Once a merge-request reaches the threshold, the merge-request is eligible for merge (from approval point of view only) and maintainers are notified. A new comment is published to make it public.

**Reminder**: an approval is (almost) irreversible. There is no `/unapprove` or
`/reject` command (an eligible rejection, emitted by a maintainer should directly
lead to closing the merge-request).

Approvals
---------

**The current number of required approvals for MPC is 3.**

FAQ
===

This section will help with questions like "What should I do if the bot says..."

Wrong issue formatting
----------------------

::

	It seems the content of the bug report 
	does not match general guidelines:


**Solution:** Your issue does not comply with guidelines, you can copy/paste the template indicated by the link. Please **do not remove any section from the template**.

Wrong merge-request formatting
------------------------------

::

	It seems the content of the merge request 
	does not match the general guidelines of this project. 
	In order to ease the reviewing process, 
	please consider the following:


**Solution:** Your merge-request does not comply with guidelines, you can copy/paste the template indicated by the link. Please **do not remove any section from the template**.
