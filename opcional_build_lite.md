Si estás usando un plan de datos móviles, descargar Visual Studio (de 8 a 11 GB) no es una opción razonable solo para compilar un navegador tan pequeño.

Por eso el navegador que has descargado es NativeNav, el cual no tiene nada que requiera obligatoriamente las herramientas súper pesadas de Microsoft. Lo único que realmente necesitas es un compilador.

He preparado una solución a medida para ti. Se creo un nuevo archivo en el proyecto llamado build_lite.bat.

Con este archivo y los pasos de abajo, podrás compilar el proyecto consumiendo menos de 105 MB en total de tus datos móviles.

El Plan "Ahorro de Datos" (Total: ~102 MB)
Descargar el compilador ligero (89 MB):

Desde tu celular o PC, entra a: w64devkit releases en GitHub
Descarga el archivo que se llama w64devkit-1.22.0.zip (o similar, asegúrate de no descargar las versiones de código fuente, sino el .zip ejecutable).
Este archivo no se instala, solo debes extraerlo (descomprimirlo) en tu PC (por ejemplo, en el escritorio o en documentos).

Abrir la consola minimalista:

Entra a la carpeta de w64devkit que acabas de descomprimir.
Haz doble clic en el programa w64devkit.exe. Se abrirá una pequeña ventana negra (esta miniconsola ya incluye el compilador g++ dentro).
Ejecutar la compilación automática (13 MB): Dentro de esa ventana negra, pega o escribe el siguiente comando para ir a la carpeta de tu código y da Enter:

bash
cd "C:\Users\robin\Documents\Antigravity Projects\NativeNav"
Luego, simplemente escribe el nombre del archivo que acabo de crearte y presiona Enter:

bash
build_lite.bat

¿Qué hará el archivo build_lite.bat?

Verificará que estás dentro de la consola ligera.
Automatizará la descarga de la utilidad nuget.exe (7 MB).
Va a descargar los recursos estrictamente obligatorios del motor WebView2 de Microsoft (6 MB).
Usará g++ (el compilador ultraligero que bajaste) para compilar inmediatamente la aplicación.
Dejará tu icono y ejecutable de NativeNav.exe funcional y listo.
¡Pruébalo! Con esto evitarás consumir más de 100 megas de tu plan móvil y te saltarás por completo cualquiera de los pesados requerimientos y registros de Visual Studio.