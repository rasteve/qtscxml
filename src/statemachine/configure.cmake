

#### Inputs



#### Libraries



#### Tests



#### Features

qt_feature("statemachine" PUBLIC
    SECTION "Utilities"
    LABEL "State machine"
    PURPOSE "Provides hierarchical finite state machines."
)
qt_feature_definition("statemachine" "QT_NO_STATEMACHINE" NEGATE VALUE "1")
qt_feature("qeventtransition" PUBLIC
    LABEL "QEventTransition class"
    CONDITION QT_FEATURE_statemachine
)