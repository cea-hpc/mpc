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

When a new issue is opened, if the `Bug` label is attached to it, the bot will ensure the description follows established guidelines listed :doc:`here<contribute_issue>`. If not, a message indicates steps to comply with these rules is posted as the first comment. You should review it and fix what necessary. Basically, if you just insert the right template, it should be fine. If not, please refer to the :ref:`FAQ<faq>` below for help.

The `Wrong Format` may be attached to the opened issue to notify you (and anyone contributing to this project) that the issue does not comply with the rules and should be fixed before letting this issue reviewed. **An issue not fixed in a reasonable time may be closed without further notice until an update is made on the description**.

Any other issue is ignored, not part of the current process. There is (for now) no other guidelines for issues. One that could come soon is an analysis of currently set labels, to ensure these are properly used depending on the context of the issue. Please choose only one label from each.

Merge Request
-------------

When a merge request is opened, its description is analyzed to check template compliance, as described :doc:`here<contribute_merge_request>`. If not, a comment indicates steps to follow in order to fix it. In the meantime, a `Wrong Format` label is used to taint it for other developers as non-compliant (yet). Still, inserting the template should be fine (available :doc:`here<contribute_merge_request>`). 

As explained in the same documentation, a merge request is reviewed and subject to approvals by peers before being merged into the main branch. The bot keeps track of the number of approvals. Once a merge request reaches the threshold, the merge request is eligible for merge.

Approvals
---------

**The current number of required approvals for MPC is 3.**

.. _faq:

