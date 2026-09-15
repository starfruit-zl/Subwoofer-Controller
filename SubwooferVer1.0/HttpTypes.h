#ifndef HTTPTYPES_H
#define HTTPTYPES_H

struct headersHttp{
  int contentLength = 0;
  char contentType[34] = {0};
  bool loginSuccess = false;
};

#endif