; proton
; Copyright (c) 2013-2019, Joshua Scoggins 
; All rights reserved.
; 
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;     * Redistributions of source code must retain the above copyright
;       notice, this list of conditions and the following disclaimer.
;     * Redistributions in binary form must reproduce the above copyright
;       notice, this list of conditions and the following disclaimer in the
;       documentation and/or other materials provided with the distribution.
; 
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
; ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; Expert system fragment that will load a given file and then generate a set of blocks containing
; the binary data of the file
(deftemplate file-data
             (slot path
                   (type LEXEME)
                   (default ?NONE))
             (slot id
                   (type SYMBOL)
                   (default ?NONE))
             (slot bytes-per-block
                   (type INTEGER)
                   (range 1 ?VARIABLE)
                   (default ?NONE)))
(deftemplate request:file-open-as-binary
             (slot path
                   (type LEXEME)
                   (default ?NONE))
             (slot id
                   (type SYMBOL)
                   (default-dynamic (gensym*)))
             (slot num-bytes-per-entry
                   (type INTEGER)
                   (range 1 ?VARIABLE)
                   (default-dynamic 1)))
(deftemplate file-block-entry
             (slot parent 
                   (type SYMBOL)
                   (default ?NONE))
             (slot base-address
                   (type INTEGER)
                   (range 0 ?VARIABLE)
                   (default ?NONE))
             (multislot contents 
                        (default ?NONE)))
(deftemplate file-walker
             (slot path
                   (type LEXEME)
                   (default ?NONE))
             (slot id
                   (type SYMBOL)
                   (default ?NONE))
             (slot index
                   (type INTEGER)
                   (default-dynamic 0))
             (slot bytes-per-block
                   (type INTEGER)
                   (range 1 ?VARIABLE)
                   (default-dynamic 1))
             (slot current
                   (type INTEGER)
                   (default ?NONE)))

(defrule open-file-for-binary-reading
         (declare (salience 10000))
         (stage (current read-file-binary-and-load-lines))
         ?f <- (request:file-open-as-binary (path ?path)
                                            (id ?id)
                                            (num-bytes-per-entry ?num))
         =>
         (retract ?f)
         (if (open ?path 
                   ?id 
                   "r") then
           (assert (file-walker (path ?path)
                                (id ?id)
                                (bytes-per-block ?num)
                                (current (get-char ?id))))
           else
           (printout stderr
                     "Could not open " ?path " for reading!" crlf)))

(defrule make-file-block-entry
         (stage (current read-file-binary-and-load-lines))
         ?f <- (file-walker (current ?val&~-1)
                            (bytes-per-block ?block)
                            (id ?id)
                            (index ?index))
         =>
         (bind ?entries
               ?val)
         (loop-for-count (- ?block 1) do
                         (bind ?tmp
                               (get-char ?id))
                         (bind ?entries
                               ?entries
                               (if (<> ?tmp -1) then ?tmp else (create$))))
         (assert (file-block-entry (parent ?id)
                                   (base-address (* ?index ?block))
                                   (contents ?entries)))
         (modify ?f
                 (index (+ ?index 1))
                 (current (get-char ?id))))
(defrule close-file-walker
         (stage (current read-file-binary-and-load-lines))
         ?f <- (file-walker (current -1)
                            (id ?id)
                            (path ?path)
                            (bytes-per-block ?c))
         =>
         (close ?id)
         (retract ?f)
         (assert (file-data (path ?path)
                            (id ?id)
                            (bytes-per-block ?c))))
(defrule retract-file-data
         (declare (salience -10000))
         (stage (current read-file-binary-and-load-lines))
         ?f <- (file-data)
         =>
         (retract ?f))
