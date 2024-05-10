# trees-area

Sometimes a man has to help his wife. This is how a husband who is a programmer can help his wife: by automating things.

She is a landscape architect. She had a task for her master thesis to calculate the number of trees and area covered by all trees on cemeteries in Warsaw, Poland. Sounds easy? Nope.

The task was not trivial, because:

1. There was only one portal which had such data - https://www.geoportal.gov.pl/
2. There was no API (of which I was aware of) which could return vector data about trees on cemeteries (like tree position, tree span in meters, nothing...)
3. Only one API (which was used by the web browser aswell) was returning basically bitmap data packed into some custom JSON format, which was not even a valid JSON, so custom parsing needed.

This API (called `getfoi`) was "optimized" by GeoPortal in such manner, so that webbrowser was first sending the size of viewport and API was returning proper JSON-aka-BMP file. That said, if you scaled up image, you could get more details, but then you had more calls to API, where every call was... extremely slow. Even if you go to this website in browser it is so much lagging and crappy. (Which is not surprise for me because it looks like GeoPortal is using some Java-based system for that haha). 

It took me some time to reverse-engineer how the `getfoi` API works but after some time I've managed to write some C programs which can:
1. Read data about cemeteries in Warsaw and save them on disk
2. Read data about trees categories in every cemetery and save them on disk
3. Poll whole cemeteries with amount of trees and their areas and save them on disk
4. Calculate total number of trees and total area which is occupied by trees in every cemetery using data from disk.

The task was even more complicated, for example because trees by nature are higher than other objects, so they cross e.g. cemetery fence. So you calculate area of trees and suddenly you are getting a value higher than total area of cemetery, which was creating a lot of confusion. Also GeoPortal has not that much precise data, so for very small cemeteries, or when trees were occluded by others (think of smaller and higher trees) results were a bit shifted and we had to apply some post-processing on them. 

Of course I wrote everything in C. I could do it in Python, but since API calls were not giving JSON-compliant results, I would need to parse them manually either way. For C/C++ I am using brilliant RDBG debugger, which helped me a lot, and in C I am just much faster than with Python.

It all worked! My wife has graduated, and I had some fun with programming aswell. :)
