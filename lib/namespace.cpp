/**
 * @file namespace.cpp
 * @brief  Brief description of file.
 *
 */
#include "angort.h"


void Namespace::list(){
    locations.listKeys();
}

void Namespace::visit(ValueVisitor *visitor){
    for(int i=0;i<entries.count();i++){
        getEnt(i)->v.receiveVisitor(visitor,getName(i));
    }
}
