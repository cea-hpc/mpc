=====================================
Run a pipeline over existing branches
=====================================

General Description
===================

Overall
-------

To keep a high level of code quality in the mainline branch, Gitlab is configured to rely on CI/CD pipelines (Continuous Integration/Continuous Development). The main objective is to avoid code regression and improve the time-to-merge for new features. A pipeline is a set of tasks (in-order) to be run (under a given configuration). Each task will return a status (success, fail, warn). It will display a badge (green, yellow, red) depending on that status, the main objective being to keep an "all-green" pipeline at any time. Now considering MPC, a pipeline can run in different scenarios:

* If a new commit is pushed to the `devel` branch, a pipeline is automatically run to assess the code quality.

* If a new merge-request is opened/updated, a pipeline is run on the head of this merge-request in a "detached" mode (such label can be seen in the pipeline UI) (not relevant to explain here why). Each time a new commit is pushed to the merge request, a new pipeline is triggered.

* Manually, through the "Run Pipeline" button, one can specify any ref (branch,
  tag, SHA...) to be run through the pipeline...

For now, all pipelines are run on a *single* Gitlab-runner, relying on a 4-node 4-core SLURM-based cluster. This means:

* Any pipeline competes with any other at the server scale. Do not panic if your pipeline does not start right away. (If more than a day of waiting, please contact us).

* Avoid pushing in a short timeframe multiple single commits **on merge-requests**. Prefer pushing once multiple commits. Any deprecated waiting pipeline on merge-requests will be canceled( ex: a first push triggers a a pipeline which, for multiple reasons, is in a waiting state. A second push triggers a new pipeline while the first still wait). If you need to push to a merge request frequently and do not want the pipeline to be run each time, you can temporarily label your merge request `CI::Disabled`.

CI/CD Tasks
-----------

Tasks are gathered into groups, where tasks inside a group are run concurrently. If a single task within a group failed, the whole pipeline is marked as failed and the remaining tasks are canceled (the running group is completed, though). An "all-green" state requires all tasks to succeed, mandatory for a merge-request to occur to be mergeable (among other conditions).

As far as this documentation is written, the MPC pipeline contains the following groups:

* Build: MPC is built only once

* Basic testing: Check MPC can still run really basic programs + config


* Regular testing: test main MPC features (MPI, OpenMP, privatization)

* Benchmarks: Assess bases of robustness within the code

* Applications: "Real" use-case application, potential hybrid MPI+X

**Reminder**: If such a pipeline allows developers to detect a major failure, an "all-green" pipeline does not necessarily mean code is perfect. That's why the peer code reviewing process applied to each merge-request is as much important as continuous integration. If one day, MPC comes with full unit-test coverage, this sentence could be mitigated but until that time, a regular complete non-regression should be run on a large cluster.

HOW-TOs
=======

Run manually a new pipeline
---------------------------

* Go to `CI/CD > Pipelines`

* click on `Run pipeline` button, top-right corner.

* Select the branch to run the pipeline on (can be a SHA1)

* Ignore the `Variables` section, not used in the current configuration

* Click on `Run the pipeline`

When going back to the main CI/CD pipeline page, a new entry should appear.

Run automatically a new pipeline
--------------------------------

A new pipeline is run automatically when:

* A new commit (or a set of commits) is pushed to a mainline branch-like devel or pt\_devel.

* Each time a new commit is added to an existing merge request.
