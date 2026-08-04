========================
Logging while developing
========================

One may need to log information on the parts one is developing.
To do so, MPC proposes a unify API for logging which incorporates multiple levels of verbosity and a clean
standardized output with the tasks, processes and nodes identified.


Understanding the verbosity levels
----------------------------------

The logging interface of MPC can be seen through the aspect of the level of verbosity considered:

1. The level 0 is the default with no logging outputs. It corresponds to the most important information to give to
   user (mainly warnings and errors).
2. The level 1 is made to communicate extra information to the user. It is mainly meant to show launching parameters
   and basic configurations.
3. The level 2 is the developer extra information stage. It allows to print more complex and specific configurations
   of the various MPC modules. It can also be used to log the most important events occurring
   during the lifetime of the program.
4. The level 3 is the debugging printing. It can be very verbose and is meant to be turn off
   when developers are not actively debugging the section of code logged.
   It is thought to track any event needed in the localisation of a bug

Those levels are directly tied to the verbose option of ``mpcrun``

.. code-block:: sh

   mpcrun      # -> warnings and errors
   mpcrun -v   # -> basic logs
   mpcrun -vv  # -> developer info
   mpcrun -vvv # -> debugging

.. note::

   The debugging messages are not available when MPC is compiled in release mode.

Using the logging interface
---------------------------

The whole interface can be accessed by including the ``mpc_common_debug.h`` header.

The logging functions are prefixed ``mpc_common_debug`` and follows the ``printf`` usage and format.

.. note::

   A final linefeed is introduced so you don't need to add ``\n`` in the format.

Defining the ``MPC_MODULE`` constant in your source file will append this string to the each logging coming from the file.
It is the preferred way to specify from where the logs are coming.

.. code-block:: c

   #define MPC_MODULE "module/submodule/subpart"

Errors
''''''

Errors can either be fatal or not. They will be displayed in red if the color support is enabled.

.. code-block:: c

   mpc_common_debug_error("This an error");  // Does not abort, just print the message
   mpc_common_debug_fatal("%s", "Aborting"); // Print the message and aborts

Warnings
''''''''

Warnings are used to signify to the user that a fallback had to be used
or that a controversial default choice have been made.
The program should be able to recover.

.. code-block:: c

   mpc_common_debug_warning("Loading %s failed: using the default value", config);

Logs
''''

Logs are (in MPC) information meant to retrieve by a user at will.
It should stay brief given only the most important information about the current execution.
As an example, it is used to display the selected launching parameters such as the number of cores or of processes.

.. code-block:: c

   mpc_common_debug_log("Using network %s for this execution", network_name);

Infos
'''''

Infos are (in MPC) more complex to understand information about the internal state of the library.
It is mostly used to track the most important events occurring in the framework (*e.g.* module initialization...).
It may gives some information about the configuration of the individual MPC modules and their state after a milestone
(*e.g.* number of threads spawned after the OpenMP initialization... )

.. code-block:: c

   mpc_common_debug_info("World communicator created with success");

.. note::

   There are shortcut to print info only from process 0 or from task 0 using the following.
   This can be useful to not spam logs when information across processes or tasks/process is identical.

   .. code-block:: c

      mpc_common_debug_info_process0("World communicator created with success");
      mpc_common_debug_info_task0("World communicator created with success");


Debugs
''''''

Debugs is the more verbose logging possible. It is meant to track event at a finer-grain while debugging.

.. code-block:: c

   mpc_common_debug("Send %p to %d", buffer, target);

.. warning::

   Most of them are meant to be deactivated most of the time.

   Please think about deactivating your logs once your debugging is finished.

   .. code-block:: c

      mpc_common_nodebug("Send %p to %d", buffer, target);


Other debugging utilities
-------------------------

MPC comes with other tools in the ``mpc_common_debug`` interface you should consider:

+-----------------------+----------------------------------------------------+
| Function              | Usage                                              |
+=======================+====================================================+
| ``assert(condition)`` | Aborts if the condition is false.                  |
|                       | It does nothing in release builds.                 |
+-----------------------+----------------------------------------------------+
| ``assume(condition)`` | Aborts if the condition is false.                  |
|                       | It only evaluates the condition in release builds. |
+-----------------------+----------------------------------------------------+
| ``not_implemented()`` | Aborts when evaluated.                             |
+-----------------------+----------------------------------------------------+
