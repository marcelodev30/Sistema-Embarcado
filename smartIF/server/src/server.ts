import express from 'express';
import * as env from 'dotenv'

env.config();
const app = express();
const port = process.env.PORT

app.use(express.json());


app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);

});
