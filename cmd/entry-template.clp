; entry.clp - entry point into the proton libs
(defgeneric defmountpoint)
(defgeneric get-mount-points)

(defglobal MAIN
           ?*mount-points* = (create$))

(defmethod defmountpoint 
  ((?name SYMBOL)
   (?path LEXEME))
  (bind ?mount (sym-cat  ?name :))
  (build (format nil "(defgeneric %s)" ?mount))
  (build (format nil "(defmethod %s () \"%s\")" ?mount ?path))
  (build (format nil "(defmethod %s ((?atoms MULTIFIELD)) (str-cat (%s) (expand$ ?atoms)))" ?mount ?mount))
  (build (format nil "(defmethod %s ($?atoms) (%s ?atoms))" ?mount ?mount))
  (bind ?*mount-points* (create$ ?*mount-points* ?mount))
  (return ?mount))

(defmethod get-mount-points 
 () 
 ?*mount-points*)


; declare proton-mount point
