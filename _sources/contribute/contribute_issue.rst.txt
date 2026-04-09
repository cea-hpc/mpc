==================
Submit a new issue
==================

-------------------
General Description
-------------------

Issues/Tickets can be used to open a discussion/submit a request to the MPC
developer team. Without them, the only alternative to communication would be
emails and it is awful. Relying on issues help developers keep track of ongoing
work while being able to refer to history when in trouble. It is thus really
important to use this mechanism heavily. But, as always, to take the best of a
technology without becoming a mess, general guidelines have to be set.

First of all, there is no restriction to the motivation for creating an issue,
it may be (almost) anything: a need for advice, a bug report, a documentation
need, a wondering, a performance inconsistency, a wish, etc... One should not
feel ashamed or afraid of writing a "note" to the community.

**The only restriction is a strict formalism when submitting a bug report**.
Because this implies a lot of work under the hood to identify the cause, it is
really important that bug reporters provide as much detail as possible. To do
so, we provide templates for it.

If an issue is stated irrelevant, it may be closed by a maintainer (or even the
author), and must be motivated, by adding a comment to justify it.

When creating a new issue, multiple boxes have to be set (as shown by the UI):

* We recommend providing a self-explanatory title to your issue, avoiding to
  "click" on it know what it is about. For instance,  `Bug in MPC` is a wrong
  title, while "MPI: Segv when calling Iprobe with more than 3 processes" is
  clearly better.

* Description supports Markdown formatting (bold, italic, title), do not
  hesitate to use them. The `Preview` button allows you to see how it renders
  once published, do not hesitate to use it too! English descriptions are
  recommended and encouraged (for international purposes). Three links at the
  bottom of the textbox allow you to (1) read the Markdown documentation, (3)
  access the quick action list (like `/approve`) and (3) attach files.

* The issue should be assigned to the person you think is in charge of answering
  to this issue, or at least able to give his point about it. If unsure, leave
  it empty.

* Labels are important to categorize your issue: is it a bug report, a feature
  request, documentation related, a discussion, about MPI, OpenMP, etc... More
  information on labels is available on GitLab.
* The "Due date" is optional


-------
HOW-TOs
-------


Create a new bug report
'''''''''''''''''''''''

While creating a bug report has to fulfill the previous guidelines, we want to
ensure new reports contain all the information allowing contributors to directly
start debugging, without having to send an email/notification each time a piece
of information is missing. First of all, note that most developers will only
address bug reports reproducible **on the mainline branch**. People are
encouraged, when bugs hit major components, to post a bug report existing and
reproducible on the main branch.

To properly fill in a bug report, please use the associated dropdown menu when
opening/editing a ticket. To ease the process of tracking bugs, it is highly
encouraged to **open a ticket for every bugfix MR**. This ticket must enclose as
much information as possible to (1) help finding a solution and (2) usable an
source of documentation if the bug arises again in the future.

Contribute to an existing issue
'''''''''''''''''''''''''''''''

Contributing to an open issue means posting a new comment to participate in the
discussion. Two ways to do it:

* You want to make a brand-new comment, related to the issue, then use the
  text-box at the bottom of the page. The same layout as for "description" above
  applies.

* You want to answer, complete, amend an existing comment, in the idea of
  creating a discussion thread, then, use the "Reply" (a small square, like a
  text bubble) in the top-right corner of the previous comment. This will
  automatically create a thread and a small textbox will appear, allowing you to
  write into this thread.

The main advantage to differentiate these two approaches is that, in a single
issue, multiple discussions can occur and gathering them in threads avoid
flooding the whole issue history line with interleaving messages.

Close an issue
''''''''''''''

Once completely answered (as judged by the author or one of the maintainers) an
issue may be closed, meaning no one can further contribute to it. It can be done
manually through the "Close Issue" in the top-right corner or through commit
message references once this commit is merged with the default branch. For more
information about that feature, please read the
:doc:`documentation<contribute_commit>`
