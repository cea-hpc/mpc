#include "lowcomm_protocol.h"
#include "lowcomm_request.h"
#include "mpc_lowcomm.h"
#include "mpc_lowcomm_msg.h"
#include "mpc_lowcomm_types.h"

#define CHECK_STATUS_ERROR(status)                          \
		do                                                  \
		{                                                   \
			if ((status)->MPC_ERROR != MPC_LOWCOMM_SUCCESS) \
			{                                               \
				return (status)->MPC_ERROR;                 \
			}                                               \
		}   while (0)

int mpc_lowcomm_sendrecv(const void *sendbuf, size_t size, int dest, int tag, void *recvbuf,
                         int src, mpc_lowcomm_communicator_t comm)
{
	mpc_lowcomm_request_t *sendreq = ((mpc_lowcomm_request_t *)mpc_lowcomm_request_alloc()) - 1;
	mpc_lowcomm_request_t *recvreq = ((mpc_lowcomm_request_t *)mpc_lowcomm_request_alloc()) - 1;

	mpc_lowcomm_request_init(sendreq, size, NULL, NULL, 0);
	mpc_lowcomm_request_init(recvreq, size, NULL, NULL, 0);

	mpc_lowcomm_isend(dest, sendbuf, size, tag, comm, sendreq, MPC_LOWCOMM_SEND_STANDARD);
	mpc_lowcomm_irecv(src, recvbuf, size, tag, comm, recvreq);

	mpc_lowcomm_status_t sts, str;

	mpc_lowcomm_wait(sendreq, &sts);
	mpc_lowcomm_wait(recvreq, &str);

	mpc_lowcomm_request_free(sendreq);
	mpc_lowcomm_request_free(recvreq);

	CHECK_STATUS_ERROR(&sts);
	CHECK_STATUS_ERROR(&str);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_wait(mpc_lowcomm_request_t *request, mpc_lowcomm_status_t *status)
{
	mpc_lowcomm_request_wait(request);
	mpc_lowcomm_commit_status_from_request(request, status);
	mpc_lowcomm_request_complete(request);

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_test(mpc_lowcomm_request_t *request, int *completed, mpc_lowcomm_status_t *status)
{
	int rc = MPC_LOWCOMM_SUCCESS;

	*completed = 0;

	/* Always make sure to start on clean grounds */
	if (status)
	{
		status->MPC_ERROR = MPC_LOWCOMM_SUCCESS;
	}

	if (request_is_null_or_cancelled(request))
	{
		*completed = 1;
		return MPC_LOWCOMM_SUCCESS;
	}

	rc = mpc_comm_progress();
	if (rc != MPC_LOWCOMM_SUCCESS)
	{
		goto err;
	}

	*completed = (request->completion_flag == MPC_LOWCOMM_MESSAGE_DONE);

	if (*completed)
	{
		mpc_lowcomm_commit_status_from_request(request, status);
		mpc_lowcomm_request_complete(request);
	}

err:
	return rc;
}

int mpc_lowcomm_waitall(int count, mpc_lowcomm_request_t *requests[], mpc_lowcomm_status_t statuses[])
{
	int all_done = 0;
	int i;

	while (!all_done)
	{
		all_done = 1;
		for (i = 0; i < count ; i++)
		{
			int done = 0;

			mpc_lowcomm_status_t *status = NULL;
			if (statuses)
			{
				status = &statuses[i];
			}

			mpc_lowcomm_test(requests[i], &done, status);
			all_done &= done;
		}

		if (!all_done)
		{
			/* Yield for coroutines */
			mpc_thread_yield();
		}
	}

	return MPC_LOWCOMM_SUCCESS;
}

int mpc_lowcomm_send(int dest, const void *data, size_t size, int tag, mpc_lowcomm_communicator_t comm)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_request_t *req = ((mpc_lowcomm_request_t *)mpc_lowcomm_request_alloc()) - 1;

	mpc_lowcomm_request_init(req, size, NULL, NULL, 0);

	mpc_lowcomm_isend(dest, data, size, tag, comm, req, MPC_LOWCOMM_SEND_STANDARD);
	mpc_lowcomm_status_t status;

	mpc_lowcomm_wait(req, &status);

	mpc_lowcomm_request_free(req);
	CHECK_STATUS_ERROR(&status);

	return rc;
}

int mpc_lowcomm_recv(int src, void *buffer, size_t size, int tag, mpc_lowcomm_communicator_t comm)
{
	int rc = MPC_LOWCOMM_SUCCESS;
	mpc_lowcomm_request_t *req = ((mpc_lowcomm_request_t *)mpc_lowcomm_request_alloc()) - 1;

	mpc_lowcomm_request_init(req, size, NULL, NULL, 0);

	mpc_lowcomm_irecv(src, buffer, size, tag, comm, req);
	mpc_lowcomm_status_t status;

	mpc_lowcomm_wait(req, &status);
	mpc_lowcomm_request_free(req);
	CHECK_STATUS_ERROR(&status);

	return rc;
}
