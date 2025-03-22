
#include <config/static-config.hh>
#include <dispatcher/api.hh>
#include <dispatcher/dispatcher.hh>
#include <dispatcher/job.hh>
#include <dispatcher/traces.hh>

namespace dispatcher {

void job_t::do_run(job_t *jb) {
    //! be caution, worker can be changed after run
    DP_TRACE(trace_current_worker, &get_current_worker());
    int rc = jb->run();
    auto &worker = get_current_worker();
    worker.return_code = rc;
    worker.exit_to_worker();
}
void job_t::do_run_sjk(job_t *jb) {
    //! be caution, worker can be changed after run
    DP_TRACE(trace_current_worker, &get_current_worker());
    int rc = jb->run();
    disable_preemption();
    auto &worker = get_current_worker();
    worker.return_code = rc;
    worker.exit_to_worker();
}

int job_t::run() {
    cmdpkt_t *cmdpkt = to_pkt();
    cmd_t cmd = cmdpkt->cmd.cmd;
    int ret = 0;
    size_t len = NET_MTU_ASSUME - sizeof(cmdpkt_t);

#define CASE(X)                               \
    case cmd_t::X:                            \
        ret = X(payload<req_##X##_t>(), len); \
        break;

    switch (cmd) {
        CASE(ping);
        CASE(array64);
        CASE(warray64);
        CASE(scan);
        CASE(get);
        CASE(vsearch);
        CASE(tpcc);
        default:
            printf("unknown cmd: %u\n", (uint8_t)cmd);
            abort();
            break;
    }

#undef CASE

    // DO NOTHING
    return ret;
}

}  // namespace dispatcher
