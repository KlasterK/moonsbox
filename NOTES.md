# Notes

## Simulation

1. Space is air here.
2. Here we call heat capacity how willingly a point changes (gives or takes) its temperature.
   And if the heat capacity is high, then the temperature of the point is harder to change. 
   If the heat capacity is zero, then the point immediately takes or gives away all the heat that 
   its neighbors give or take away. 
3. Thermal conductivity is how willingly a point shares its temperature with others. 
   If it is 1, then the point immediately equalizes its temperature and its neighbor's 
   (if we imagine that there are only 2 points in the neighborhood). 
   If it is 0, then it never shares its temperature.

## Saving Specification (1.0.1-alpha)

Requirements:

1. Save must be a pickled mapping.
2. Mapping must have 'application' key equal to 'moonsbox' string.
3. Mapping must have 'version' key and it must be a sequence. 
   It must have in it the current save version (string '1.0.1-alpha') and may have any versions
   for which the save is compatible.
5. Mapping must have 'lists' key and it must be a sequence.
   It must be a 2-dimensional rows-first sequence with straight shape 
   and it must only contain instances of BaseMaterial.

Example save (unpickled):

```py
{
    'application': 'moonsbox',
    'version': ('1.0.1-alpha',),
    'lists': [
        [Space(...), Space(...), Water(...)],
        [Space(...), Sand(...),  Space(...)],
        [Space(...), Space(...), Space(...)],
    ],
}
```
