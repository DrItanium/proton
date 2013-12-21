; generates a startup script combined with a makefile
(load* /tmp/generate-startup-script.input)
(defrule build-script-contents
 ?f <- (root ?program ?location)
 =>
 (retract ?f)
 (format t "#!/bin/bash%n# Root is '%s'%nelectron -f2 \"%s/entry.clp\" $@%n"
  ?location ?location))

(batch* lib/reset-run-exit.clp)

