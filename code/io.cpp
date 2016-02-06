#include <fcntl.h>
#include <fstream>
#include "graph.h"
#include "io.h"
#include <iostream>
#include "param.h"
#include <string>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>
#include "util.h"
#include <vector>

namespace std {

tuple<uint32_t,vector<edge>,vector<edge>>
  read_edges(string filename, const unordered_set<uint32_t>& train_gids) {
  // read edges into memory
  cout << "Reading edges from: " << filename << endl;

  vector<edge> train_edges;
  vector<edge> test_edges;

  // get file size
  struct stat fstatbuf;
  int fd = open(filename.c_str(), O_RDONLY);
  fstat(fd, &fstatbuf);

  // memory map the file
  char *data = (char*) mmap(NULL, fstatbuf.st_size, PROT_READ,
                            MAP_PRIVATE|MAP_POPULATE, fd, 0);
  madvise(data, fstatbuf.st_size, MADV_SEQUENTIAL);

  if (data < 0) { // mmap failed
    panic("mmap'ing graph file failed");
    close(fd);
  }

  // read edges from the file
  uint32_t i = 0;
  uint32_t line = 0;
  uint32_t max_gid = 0;
  char src_type, dst_type, e_type;
  while (i < fstatbuf.st_size) {
    // field 1: source id
    uint32_t src_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      src_id = src_id * 10 + (data[i] - '0');
    }

    i++; // skip delimiter

    // field 2: source type
    src_type = data[i];
    i += 2; // skip delimiter

    // field 3: dest id
    uint32_t dst_id = data[i] - '0';
    while (data[++i] != DELIMITER) {
      dst_id = dst_id * 10 + (data[i] - '0');
    }
    i++; // skip delimiter

    // field 4: dest type
    dst_type = data[i];
    i += 2; // skip delimiter

    // field 5: edge type
    e_type = data[i];
    i += 2; // skip delimiter

    // field 7: graph id
    uint32_t graph_id = data[i] - '0';
    while (data[++i] != '\n') {
      graph_id = graph_id * 10 + (data[i] - '0');
    }

    if (graph_id > max_gid) {
      max_gid = graph_id;
    }

    i++; // skip newline

    // add an edge to memory
    if (train_gids.find(graph_id) != train_gids.end()) {
      train_edges.push_back(make_tuple(src_id, src_type,
                                       dst_id, dst_type,
                                       e_type, graph_id));
    } else {
      test_edges.push_back(make_tuple(src_id, src_type,
                                      dst_id, dst_type,
                                      e_type, graph_id));
    }

    line++;
  }

  close(fd);

#ifdef VERBOSE
    for (uint32_t i = 0; i < edges.size(); i++) {
      cout << "Edge " << i << ": ";
      print_edge(edges[i]);
      cout << endl;
    }
#endif

  return make_tuple(max_gid + 1, train_edges, test_edges);
}

tuple<vector<vector<uint32_t>>, vector<double>, double>
  read_bootstrap_clusters(string bootstrap_file) {
  int nclusters;
  double global_threshold;
  ifstream f(bootstrap_file);
  string line;
  stringstream ss;

  getline(f, line);
  ss.str(line);
  ss >> nclusters >> global_threshold;
  vector<double> cluster_thresholds(nclusters);
  vector<vector<uint32_t>> clusters(nclusters);

  for (int i = 0; i < nclusters; i++) {
    getline(f, line);
    ss.clear();
    ss.str(line);

    double cluster_threshold;
    ss >> cluster_threshold;
    cluster_thresholds[i] = cluster_threshold;

    uint32_t gid;
    while (ss >> gid) {
      clusters[i].push_back(gid);
    }
  }

  return make_tuple(clusters, cluster_thresholds, global_threshold);
}

}
