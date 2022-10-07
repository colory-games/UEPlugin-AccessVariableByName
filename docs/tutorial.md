# Tutorial

## Introduction Video

> TODO: Add Video link

## Get Variables by Name

'Get Variables by Name' node gets variables by specifing the name and the target object.
This node analyzes the name and the target (static analysis), and changes the node's pin type to the same variable type automatically.
It is useful for checking if the pin type is valid on the compilation time.

> TODO: Add image which compares 

### Usage

1. Search and place 'Get Variables by Name' node on the Blueprint editor.
2. Connect node's pins to the other pins or input a literal string.
  * Target: An object reference which may have an desired member variables.
  * Var Name: The name of an desired variable. This pin's value support a custom syntax to access the nested variable. Only literal string is allowed, and error if you connect pin to the other node.
  * Success: Output `True` if an desired member variable is successfully acquired.
  * <variable-name>: Output the value of an desired member variable. The pin name will be same as a name of an desired member variable. This pin will be hidden if an desired member variable does not exist.


## Custom Syntax

'Var Name' pin of the node support a custom syntax to get/set a deep nested variable.

### Access to a structure/object member variable

You can access to structure/object member variable directly by specifing the dot separated name.
In the case you want to access the member variable `X` which belongs to the `StructVar` with type of structure `Vector`, specify below literal string.

```
StructVar.X
```

#### Compare to the Blueprint script

Compared to the Blueprint script on the vanila Unreal Engine, you can get the desired variable more directly.
If the variable is deeper, this syntax will be much more powerful.

> TODO: Add image to explain

### Access to an array/map element

You can access to the element of array/map by specifing the index or key.
In the case you want to access the 2nd element of the array member variable `ArrayVar`, specify the index `1` surrounded by `[]`.

```
ArrayVar[1]
```

Note: Array index is 0-based index. If the specified index is out of range, 'Success' pin will output `False` value.

#### Compare to the Blueprint script

> TODO: Add image to explain

In the case you want to access the element of the set member variable `SetVar` whose key is `"Key"`, specify the key `"Key"` surrounded by `[]`.

```
SetVar["Key"]
```

Note: If the element is not found, 'Success' pin will output `False` value.

Node: Support 

#### Compare to the Blueprint script

> TODO: Add image to explain

### Combined syntax

Of course, you can combine these syntax as follows.

> Add: Images to show example
