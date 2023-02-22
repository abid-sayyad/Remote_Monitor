const express = require("express")
const bodyParser = require("body-parser")
const mongoose = require("mongoose")
const dotenv = require("dotenv")

const PORT = 80||443

const app = express()

//setup dotenv
dotenv.config()

//middleware functions
app.use(bodyParser.json())
app.use(bodyParser.urlencoded({ extended: false }))

//setup mongoose
const mongodbConnString = "mongodb+srv://hellouser:FkUwNcKaxMugCX4m@patient0.jzbvmqt.mongodb.net/Patient0"
// console.log(mongodbConnString);

mongoose.connect(mongodbConnString)

mongoose.connection.on("error", function(error) {
  console.log(error)
  console.log("fuck you")
})

mongoose.connection.on("open", function() {
  console.log("Successfully connected to MongoDB Atlas database.")
})

//middleware function
app.use(require("./routes/keycodes/keycode"))

app.listen(PORT, function () {
  console.log(`Server app listening on port ${PORT}`)
})
