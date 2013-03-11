;------------------------------------------------------------------------------
;Copyright (c) 2013, Joshua Scoggins 
;All rights reserved.
;
;Redistribution and use in source and binary forms, with or without
;modification, are permitted provided that the following conditions are met:
;    * Redistributions of source code must retain the above copyright
;      notice, this list of conditions and the following disclaimer.
;    * Redistributions in binary form must reproduce the above copyright
;      notice, this list of conditions and the following disclaimer in the
;      documentation and/or other materials provided with the distribution.
;    * Neither the name of Joshua Scoggins nor the
;      names of its contributors may be used to endorse or promote products
;      derived from this software without specific prior written permission.
;
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
;ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
;WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;DISCLAIMED. IN NO EVENT SHALL Joshua Scoggins BE LIABLE FOR ANY
;DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
;(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
;LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
;ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;------------------------------------------------------------------------------
; WidgetElement.clp - Base class of all widgets 
; 
; Written by Joshua Scoggins 
; Started on 3/11/2013
;------------------------------------------------------------------------------
(defclass widget::WidgetElement 
  "Base class of all widgets in the adventure engine"
  (is-a Object)
  (slot position-x (type NUMBER))
  (slot position-y (type NUMBER))
  (slot width (type NUMBER))
  (slot height (type NUMBER))
  (slot reference-count (type NUMBER) (range 0 ?VARIABLE))
  (multislot children (type SYMBOL))
  (multislot valid-events (type SYMBOL))
  (message-handler declare-handler primary)
  (message-handler raise-event primary))
;------------------------------------------------------------------------------
(defmessage-handler widget::WidgetElement declare-handler primary 
                    (?name ?fn)
                    (bind ?we-name (gensym*))
                    (make-instance ?we-name of WidgetEvent 
                                   (event-name ?name)
                                   (function-to-call ?fn)
                                   (parent ?self:id)
                                   (reference-count 1))
                    (bind ?result (member$ ?name ?self:valid-events))
                    (if (not ?result) then
                      (slot-direct-insert$ valid-events 1 ?name ?we-name )
                      else
                      (printout werror "ERROR: Firing of multiple events not supported yet"  crlf)
                      (halt))
                    (return ?we-name))
;------------------------------------------------------------------------------
(defmessage-handler widget::WidgetElement raise-event primary
                    (?name $?args)
                    (bind ?offset (member$ ?name ?self:valid-events))
                    (if (not ?offset) then
                      (printout werror "ERROR: given event " ?name 
                                " does not exist. Halting" crlf)
                      (halt)
                      else
                      (send (instance-address * (symbol-to-instance-name 
                                                  (nth$ (+ 1 ?offset)
                                                        ?self:valid-events)))
                            raise-event $?args)))
;------------------------------------------------------------------------------
