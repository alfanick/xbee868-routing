#ifndef PUT_TEST_COMMON_H
#define PUT_TEST_COMMON_H

//! setParameterHelper
#define SET_EDGE(n, a, b, g, e, r) { \
  XbeeRouting::Parameters* tmp = n.edge(XbeeRouting::Address(a), XbeeRouting::Address(b));\
  tmp->good = g; tmp->errors = e; tmp->retries = r; }

#define SET_EDGE_WITH_DELAY(n, a, b, g, e, r, d) { \
  XbeeRouting::Parameters* tmp = n.edge(XbeeRouting::Address(a), XbeeRouting::Address(b));\
  tmp->good = g; tmp->errors = e; tmp->retries = r; tmp->delay=d; }

#endif
