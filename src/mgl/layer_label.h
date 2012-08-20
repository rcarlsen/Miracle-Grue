/* 
 * File:   layer_label.h
 * Author: Dev
 *
 * Created on August 20, 2012, 9:26 AM
 */

#ifndef LAYER_LABEL_H
#define	LAYER_LABEL_H

namespace mgl {

class LayerLabel {
public:
    enum TYPE {
        TYP_RAFT,
        TYP_MODEL,
        TYP_INVALID
    };
    LayerLabel(TYPE t = TYP_INVALID, int v = -1) 
            : myType(t), myValue(v) {}
    bool isRaft() const {
        return myType == TYP_RAFT; 
    }
    bool isModel() const {
        return myType == TYP_MODEL;
    }
    bool isInvalid() const {
        return myType == TYP_INVALID;
    }
    bool isValid() const {
        return !isInvalid();
    }
    TYPE myType;
    int myValue;
};

}



#endif	/* LAYER_LABEL_H */

