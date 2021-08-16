# MPC_OpenMP tools

## How to use OMP task tracing

1. Edit your code, example :
```
# pragma omp parallel
{
    mpc_omp_task_trace_begin();

    # pragma omp single
    {
        mpc_omp_task("my task label", 0);
        # pragma omp task
        {
            [...]
        }
    }

    mpc_omp_task_trace_end();
}
```
2. Set the environnement variable `MPCFRAMEWORK_OMP_TASK_TRACE=1`.
3. Run your program
4. Move generated `.dat` files to a trace directory of your choice
5. Run post-processor on the trace directory

## Catapult : Chrome Trace Event (CTE) (Gantt diagram)
The file `sample-trace.json` contains a sample CTE trace.    
You can visualise it on any chromium-based navigator, through Catapult by navigating to the `about:tracing` URL.

The `trace_to_cte.py` allows to convert any MPC_OpenMP task trace to be converted to a CTE trace, and thus, to be visualised in Catapult.

## .dot graphs
The `trace_to_dot.py` allows to convert any MPC_OpenMP task trace to a `.dot` file format.    
See `python3 trace_to_dot.py --help`.

## Statistics
The `trace_stats.py` retrieve many stats from a run, by extracting informations for a MPC_OpenMP task trace.
