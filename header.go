package main

import "fmt"
import "net/mail"
import "net/textproto"
import "os"
import "strconv"
import "strings"

var parse = verbatim
var verbose = false

func addresses(key string, value string) {
  if list, err := mail.ParseAddressList(value); err == nil {
    for _, address := range list {
      verbatim(key, address.Address)
    }
  }
}

func time(key string, value string) {
  if time, err := mail.ParseDate(value); err == nil {
    verbatim(key, strconv.FormatInt(time.Unix(), 10))
  }
}

func verbatim(key string, value string) {
  if verbose {
    fmt.Printf("%s: %s\n", key, value)
  } else {
    fmt.Println(value)
  }
}

func extract(file *os.File, headers[]string) {
  if msg, err := mail.ReadMessage(file); err == nil {
    for _, key := range headers {
      for _, value := range msg.Header[key] {
        parse(key, value)
      }
    }
  }
}

func options(args []string) []string {
  for i, arg := range(args) {
    if arg[0] != '-' {
      return args[i:]
    } else if arg == "--" {
      return args[i + 1:]
    }

    for _, option := range(arg[1:]) {
      switch option {
        case 'a':
          parse = addresses
        case 't':
          parse = time
        case 'v':
          verbose = true
        default:
          usage()
      }
    }
  }
  return nil
}

func usage() {
  fmt.Fprintf(os.Stderr, `Usage: %s [OPTION]... HDR[,HDR]... [FILE]...
Extract the unfolded content of specified headers from each message given
as an argument or from the message supplied on stdin.

Options:
  -a    parse address list headers and print individual addresses
  -t    parse date headers and convert to decimal unix timestamps
  -v    print headers with their keys as well as their values
`, os.Args[0])
  os.Exit(64)
}

func main() {
  var args = options(os.Args[1:])
  var headers []string

  if len(args) == 0 {
    usage()
  }

  headers = strings.Split(args[0], ",")
  for i, h := range headers {
    headers[i] = textproto.CanonicalMIMEHeaderKey(strings.TrimSpace(h))
  }

  if len(args) == 1 {
    extract(os.Stdin, headers)
  }

  for _, path := range(args[1:]) {
    file, err := os.Open(path)
    if (err != nil) {
      fmt.Fprintln(os.Stderr, err)
      os.Exit(1)
    }
    extract(file, headers)
    file.Close()
  }
}
