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

; Expert system fragment that will load a given file and then invoke a given function 
; on each line of the file. 
(deftemplate file-walker
             (slot path
                   (type LEXEME)
                   (default ?NONE))
             (slot id
                   (type SYMBOL)
                   (default ?NONE))
             (slot line-number
                   (type INTEGER)
                   (default-dynamic 0))
             (slot current-line
                   (type LEXEME)
                   (default ?NONE)))
(deftemplate request:file-open
             (slot path
                   (type LEXEME)
                   (default ?NONE))
             (slot id
                   (type SYMBOL)
                   (default-dynamic (gensym*))))
(deftemplate line-entry
             (slot parent 
                   (type SYMBOL)
                   (default ?NONE))
             (slot index
                   (type INTEGER)
                   (range 0 ?VARIABLE)
                   (default ?NONE))
             (slot line
                   (type LEXEME)
                   (default ?NONE))
             (multislot contents 
                        (default ?NONE)))
(defrule open-file-for-reading
         (declare (salience 10000))
         (stage (current read-file-and-load-lines))
         ?f <- (request:file-open (path ?path)
                                  (id ?id))
         =>
         (retract ?f)
         (if (open ?path 
                   ?id 
                   "r") then
           (assert (file-walker (path ?path)
                                (id ?id)
                                (current-line (readline ?id))))
           else
           (printout stderr
                     "Could not open " ?path " for reading!" crlf)))

(defrule make-line-entry
         (stage (current read-file-and-load-lines))
         ?f <- (file-walker (current-line ?val&~EOF)
                            (line-number ?index)
                            (id ?id))
         =>
         (modify ?f
                 (line-number (+ ?index 1))
                 (current-line (readline ?id)))
         (assert (line-entry (parent ?id)
                             (index ?index)
                             (line ?val)
                             (contents (explode$ ?val)))))
(defrule close-file-walker
         (stage (current read-file-and-load-lines))
         ?f <- (file-walker (current-line EOF)
                            (id ?id))
         =>
         (close ?id)
         (retract ?f))
