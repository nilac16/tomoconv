#include "dbinfo.h"
#include "error.h"


tomo::dbinfo::dbinfo()
{

}


void tomo::dbinfo::construct(pugi::xml_node root)
{
    pugi::xml_node node;

    node = xchild(root, "databaseUID");
    uid() = node.text().as_string();
    root = xchild(root, "creationTimestamp");
    node = xchild(root, "date");
    date() = node.text().as_string();
    node = xchild(root, "time");
    time() = node.text().as_string();
}
