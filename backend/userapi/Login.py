from fastapi import APIRouter

from .schema import *

import Db

router = APIRouter(prefix="/login", tags=["Login"])

