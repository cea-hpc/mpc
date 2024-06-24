==========================
Understand label semantics
==========================

General description
===================

This page intends to describe two concepts this Gitlab instance is relying on. Milestones are attached to issues/merge-requests to easily gather them under a specific "roadmap" item. Second, labels can also be attached to issues/merge-requests and help readers to easily categorize contents, depending on multiple criteria (Topic, progress, priority...).

TL;DR
=====

Labels to be used when managing issue/MR (consider picking one, and only one, from each category):

==========================	=======	=====	=========================================================================================================
Label				Issue	MR	Def
==========================	=======	=====	=========================================================================================================
`Process::Pending`		**X**	**X**	Newly opened, unassigned ticket / MR 
`Process::WIP`			**X**	**X**	ticket / MR under active development (assignee = dev actually working on it) 
`Process::Need Reviews`		*Avoid*	**X**	Some comments have been raised and need to be addressed before stating this work as `Progress::Completed`
`Process::Completed`		**X**	**X**	Job. Is. Done \o/ 
`Process::Need discussion`	**X**		People owning this issue expects people to give their point before going further
`Priority::High`		**X**		High-priority issue, to be addressed in forthcoming days/weeks. Keep its use as low as possible
`Priority::Low`			**X**		Priority lower than normal priority (default, no label)
`Topic::Build / Install`	**X**		MPC build or installation
`Topic::C++`			**X**		C++ related
`Topic::Fortran`		**X**		Fortran-related
`Topic::MPI`			**X**		MPI related
`Topic::OpenMP`			**X**		OpenMP related
`Topic::Performance`		**X**		Performance related
`Topic::Release`		**X**		Release related
`Topic::Others`			**X**		Unclassifiable (and not worth to be)
`Bug`				**X**		Special label to track down bug-related issues
==========================	=======	=====	=========================================================================================================

"Automatic labels" managed by bots:

==============================	======	=====	=============================================================================================
Label				Issue 	MR 	Def.
==============================	======	=====	=============================================================================================
`Wrong Format`			**X**	**X**	Issue/MR does not follow general guidelines
`Status::Waiting for Approval`		**X**	Once flagged with `Process::Completed``, notify this MR as waiting for approvals to be merged
`Status::Approved`			**X**	Enough approvals to be merged (**no regard for other requirements like CI...**)
`Status::Rejected`		**X**	**X**	Issue / MR to be closed for justified reasons
==============================	======	=====	=============================================================================================

Milestones 
----------

MPC timeline will be split under milestones, to keep track globally of MPC work progression. Categorized under "deliveries" tag, it will gather under the same milestone any content due to a specified date. This is unrelated to a project release (as multiple releases could occur in a milestone).

It could have been interesting to map a milestone to an MPC revision. But the visibility offered by milestones would not be fully exploited by the current MPC revision process (a new release not being on a regular basis). While a milestone let people easily know what is expected to be discussed/fixed/solved in a given time frame. For instance, the `2020 Deliveries` milestone will be attached to content planned to be addressed during the 2020 year. A "special" milestone, for contents unrelated to any due date, can be attached to `Go with the flow` before a decision is made. For instance, a bug submission can be attached to this milestone before any review has been made. Once a bug severity is defined (and its urge to be fixed known), a proper milestone should be set to this bug.

To choose the proper milestone, take one or two minutes to clearly identify when this work/ticket/pull-request is supposed to be addressed. If it should be done for the current year, use the appropriate milestone.

Labels
------

To ease managements of issues/merge-requests, Gitlab comes with "labels" which can be attached to them to let developers know what they are about. A label helps to have, at a glance, a clear idea of the situation. For MPC, three "types" of labels exist:

* **`priority` labels**: Highlights specific issues to order them. "Normal" priority is the default. There is two specific labels to distinguish otherwise:

================	=========================================================================================================================================================================================================
label name 		description
================	=========================================================================================================================================================================================================
`Priority::High`	Express an urgent question/fix that should be reviewed by developers in a short term timeframe (under a week, ideally), the real idea is to keep the number of tickets with this label as low as possible
`Prioritty:Low`		Indicates secondary resources, to be listed but not needing an immediate focus or even long-terme based (codestyle, colouring, etc.)
================	=========================================================================================================================================================================================================

**Priority labels should only be attached to TICKETS**. MR do not need to be priorized as there is no capacity to order them automatically (an MR merged to devel will invalidate all others because of non-fast-forwarding). 

* **`Topic` labels**: Help identifying the target of the ssue. Component non-experts can then easily discard them and focused on others, where their skills are more efficient). One can distinguish four different major components:

========================	==================================================================
label name			description
========================	==================================================================
`Topic::Build / Install`	Related to MPC build or installation
`Topic::C++`			Related to C++ support (may implies thread-based most of the time)
`Topic::Fortran`		as above but for Fortran
`Topic::Lowcomm`		Relative to communication layer
`Topic::MPI`			Relative to MPI layer
`Topic::OpenMP`			Relative to OpenMP layer
`Topic::Performance`		Relative to global MPC performance
`Topic::Release`		Relative to the next release procedure
`Topic::Other`			Unclassifiable topic
========================	==================================================================

**Topic labels should be used only for TICKETS**. It helps giving a hint to the content, no need for MR to carry such info. If necessary, open a ticket and link it to the MR :wink:  We encourage to use only one topic per ticket to help the sorting. If not possible, try to keep the number of topics as minimal as possible.

* **`Process` labels**, to help distribute work among the dev team, along with tracking down the current ticket state at a glance:

===========================	====================================================================================================================================
label name			description
===========================	====================================================================================================================================
`Process::Pending`		Default tag, just-opened tickets, nobody currently cares about it
`Process::WIP`			This marks the current ticket/MR as under active development. The Assignee field should be set the people actively working on it.
`Process::Completed`		WOrk. Is. Done. \o/
`Process::Need Reviews`		Additional: Work expected to be completed but comments from other devs raised a concern and should be addressed before going further
`Process::Under Discussion`	Flag the ticket/MR to be discussed with more than 1 people (next MPC show?)
===========================	====================================================================================================================================

**Process labels** are expected to be used by both tickets MRs. If not set, the default `Progress::Pending`` should be given.

* **Other labels**:

==========	===============================================
label name	description
==========	===============================================
`Bug`		For tickets, flag them as bugs, easy to filter.
==========	===============================================

* **"Automatic" labels**: These labels are not intended to be used by humans and are
  set/removed by a bot, in charge of enforcing the guidelines:

==============================	=======================================================================================================================================================================================================
label name			description
==============================	=======================================================================================================================================================================================================
`Wrong Format`			Used for issues / merge-requests not following standards published onto Wiki pages. This label should be followed with a comment detailing the procedure to make this label removed (still by the bot)
`Status::Waiting for Approval`	Added by the bot when an MR is `Process::Completed` but not enough approvals to be merged yet
`Status::Approved`		Added by the bot when an MR reached enough approvals to be merged (with no regard to other requirements)
`Status::Rejected`		Manually added if a proper reason is given to reject this ticket/MR (implies closing)
==============================	=======================================================================================================================================================================================================

HOW-TOs
=======

Create a new milestone
----------------------

To be written, someday, even if it seems super straight-forward to create a new
label (`Issues > Milestones > New Milestone`)

Create a new label
------------------

To be written someday, even if creating a new label is not a common operation. Be sure to set an appropriate scope for the new label (project vs group). A group labels can be reused across all projects within the same group.