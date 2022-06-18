#!/usr/bin/python3

from alive_progress import alive_bar
import argparse
import os
import subprocess
import csv
import shutil

FLUSH_CACHE_PATH = "./lib/tools/flush_cache"

PINTOOL = ""

PERF_BENCH = ['perf', 'stat', '-x,']
PERF_EVENTS = [
    'cycles',
    'L1-dcache-loads-misses',
    'LLC-load-misses',
    'node-loads',
    'node-stores',
    'node-loads-misses',
    'node-stores-misses',
]
PERF_EXEC = PERF_BENCH + [f'-e {event}:u' for event in PERF_EVENTS]

PAPI_EVENTS = ["PAPI_L2_DCM", "PAPI_SR_INS", "PAPI_LD_INS", "PAGE-FAULTS"]

PINTOOL_EVENTS = [
    "touches_private",
    "touches_CH",
    "touches_CB",
    "touches_CC",
    "touches_NC",
    "touches_amount",
    "touches_clusterSD",
    "touches_hop_element",
    "sharing_private",
    "sharing_CH",
    "sharing_CB",
    "sharing_CC",
    "sharing_NC",
    "sharing_amount",
    "sharing_clusterSD",
    "sharing_hop_element",
    "avg_sharing_degree",
    "sd_sharing_degree",
    "avg_write_degree",
    "sd_write_degree",
    "avg_write_ratio",
    "sd_write_ratio",
    "avg_shared_write_ratio",
    "sd_shared_write_ratio",
    "footprint",
    "sd_thread_footprint",
]
PINTOOL_EXEC = [
    'pin', '-t', os.path.abspath('./lib/instrumentation/src/obj-intel64/PinTool.so'), '--']

OMP_PLACES = ["sockets", "cores", "threads"]
OMP_PROC_BIND = ["master", "close", "spread"]

HEADER = ['OMP_PLACES', 'OMP_PROC_BIND']


parser = argparse.ArgumentParser(""" ./run-benchmark.py [OPTIONS]
Benchmark suite tool, for testing NUMA architecture.
This script wraps Perf to perform benchmark over a given input binary file.
It is possible also to perform benchmarks with Papi and Pintool.

HPC project 2021/2022 - UNISA
Authors: Antonio De Caro, Salvatore Fasano, Gerardo Gullo
""")
parser.add_argument('-f', '--conf-path', metavar='<path>', type=str,
                    help='path of a file containing the list of benchmarks', default='./benchmark.conf')
parser.add_argument('-n', '--iterations', metavar='<integer>', type=int,
                    help='number of iterations', default=1)
parser.add_argument('-o', '--out-folder', metavar='<path>', type=str,
                    help='the output folder', default='.')
parser.add_argument('-c', '--flush-cache', action='store_true',
                    help='flush the cache before each benchmark')
parser.add_argument('-p', '--with-pintool', action='store_true',
                    help='use Pintool to perform benchmark')
parser.add_argument('-P', '--with-papi', action='store_true',
                    help='use Papi to perform benchmark')

args = parser.parse_args()


def flush_cache():
    if (args.flush_cache):
        subprocess.run([FLUSH_CACHE_PATH])


def run_perf_benchmark(bench_exec, bench_name):
    def parse_row(out):
        res = [el for el in map(lambda s: s.strip(),
                                out.stderr.decode('utf-8').split(',')) if el]
        time = out.stdout.decode('utf-8').strip()
        vals = {}
        val = res[0]
        for key in res[1:]:
            key = key.replace(':u', '')
            if key in PERF_EVENTS:
                if val == '<not counted>':
                    vals[key] = ''
                else:
                    vals[key] = val
            else:
                val = key

        return [time] + [vals[e] for e in PERF_EVENTS]
    with open(f'{args.out_folder}/results/{bench_name}_perf.csv', 'w') as f:
        writer = csv.writer(f)
        writer.writerow(HEADER + ['nanoseconds'] + PERF_EVENTS)

        title = f"[*] Perf {bench_name}"

        with alive_bar(args.iterations * (len(OMP_PLACES) * len(OMP_PROC_BIND) + 1), ) as bar:
            for _ in range(args.iterations):
                os.environ['OMP_NUM_THREADS'] = '1'
                bar.title(f'{title}: x, x')
                flush_cache()
                out = subprocess.run(
                    PERF_EXEC + [bench_exec], capture_output=True)
                del os.environ['OMP_NUM_THREADS']
                row = parse_row(out)
                writer.writerow(['x', 'x'] + row)
                bar()

                for (place, bind) in [(a, b) for a in OMP_PLACES for b in OMP_PROC_BIND]:
                    os.environ['OMP_PLACES'] = place
                    os.environ['OMP_PROC_BIND'] = bind
                    bar.title(f"{title}: {place}, {bind}")
                    flush_cache()
                    out = subprocess.run(
                        PERF_EXEC + [bench_exec], capture_output=True)
                    row = parse_row(out)
                    writer.writerow([place, bind] + row)
                    bar()


def run_papi_benchmark(bench_exec, bench_name):
    os.environ['WITH_PAPI'] = 'True'
    os.mkdir(f"{args.out_folder}/results/temp/")

    with open(f'{args.out_folder}/results/{bench_name}_papi.csv', 'w') as f:
        writer = csv.writer(f)
        writer.writerow(HEADER + PAPI_EVENTS)

        title = f"[*] Papi {bench_name}"
        with alive_bar(args.iterations * (len(OMP_PLACES) * len(OMP_PROC_BIND) + 1), ) as bar:

            for _ in range(args.iterations):
                bar.title(f'{title}:x, x')
                os.environ['OMP_NUM_THREADS'] = '1'
                flush_cache()
                os.environ['RESULT_DIR'] = os.path.abspath(
                    f'{args.out_folder}/results/temp/{bench_name}.csv')
                subprocess.run([bench_exec], stdout=subprocess.DEVNULL)
                del os.environ['OMP_NUM_THREADS']
                bar()

                for (place, bind) in [(a, b) for a in OMP_PLACES for b in OMP_PROC_BIND]:
                    bar.title(f'{title}:{place}, {bind}')
                    os.environ['OMP_PLACES'] = place
                    os.environ['OMP_PROC_BIND'] = bind
                    flush_cache()
                    subprocess.run([bench_exec], stdout=subprocess.DEVNULL)
                    bar()

        with open(f'{args.out_folder}/results/temp/{bench_name}.csv', 'r') as t:
            t.readline()  # remove headers
            for i in range(args.iterations):
                writer.writerow(['x', 'x'] + t.readline().split()[2:])
                for place in OMP_PLACES:
                    for bind in OMP_PROC_BIND:
                        writer.writerow(
                            [place, bind] + t.readline().split()[2:])

    del os.environ['WITH_PAPI']
    shutil.rmtree(f"{args.out_folder}/results/temp/")


def run_pintool_benchmark(bench_exec, bench_name):
    os.mkdir(f"{args.out_folder}/results/{bench_name}")

    with open(f'{args.out_folder}/results/{bench_name}_pintool.csv', 'w') as f:
        writer = csv.writer(f)
        writer.writerow(HEADER + PINTOOL_EVENTS)

        title = f"Pintool {bench_name}"
        with alive_bar(len(OMP_PLACES) * len(OMP_PROC_BIND)) as bar:
            writer.writerow(['x', 'x'])

            for (place, bind) in [(a, b) for a in OMP_PLACES for b in OMP_PROC_BIND]:
                bar.title(f'{title}:{place}, {bind}')
                os.environ['OMP_PROC_BIND'] = bind
                os.environ['OMP_PLACES'] = place
                flush_cache()
                subprocess.run(
                    PINTOOL_EXEC + [bench_exec], cwd=f"{args.out_folder}/results/",
                    stdout=subprocess.DEVNULL)
                with open(f'{args.out_folder}/results/{bench_name}/metrics.csv', 'r') as t:
                    lines = t.readlines()[-2:]  # remove headers
                    writer.writerow([place, bind] + lines[1].split())
                bar()

    shutil.rmtree(f"{args.out_folder}/results/{bench_name}")


if __name__ == '__main__':
    if os.path.isdir(f"{args.out_folder}/results"):
        shutil.rmtree(f"{args.out_folder}/results/")
    os.mkdir(f"{args.out_folder}/results/")

    conf = []
    with open(args.conf_path) as f:
        line = f.readline()
        while line:
            line = line.split('#')[0]
            if line.strip():
                conf.append(line)
            line = f.readline()

    try:
        for line in conf:
            bench_exec = line.strip()
            tokens = line.split(' ')
            bench_name = tokens[0].split('/')[-1].strip()

            print(bench_exec, bench_name)
            run_perf_benchmark(bench_exec, bench_name)
            if args.with_papi:
                run_papi_benchmark(bench_exec, bench_name)
            if args.with_pintool:
                run_pintool_benchmark(bench_exec, bench_name)
    except KeyboardInterrupt:
        print("[!] Terminating")
