; declaration of the stage template

(deftemplate stage
             "Stage control fact, used to separate groups of rules and introduce order"
             (slot current
                   (type SYMBOL)
                   (default ?NONE))
             (multislot rest
                        (type SYMBOL)))

(defrule next-stage
         "Advance to the next stage"
         (declare (salience -10000))
         ?f <- (stage (rest ?next $?rest))
         =>
         (modify ?f
                 (current ?next)
                 (rest ?rest)))
